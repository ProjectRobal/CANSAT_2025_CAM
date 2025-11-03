/*

I2C pins


*/

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

#include <freertos/FreeRTOS.h>
#include <esp_log.h>

#include <stdio.h>
#include <dirent.h>
#include <esp_timer.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>
#include <driver/ledc.h>
#include <driver/gptimer.h>

#include <freertos/queue.h>
#include <driver/uart.h>


#include <esp_camera.h>

#include "esp_littlefs.h"

#include "rom/crc.h"

#include "config.h"


volatile bool stopCamera=false;
volatile bool startCamera=false;

enum LED_STATUS_BITS
{
    STATUS_OK=0,
    STATUS_CAMERA_INIT_FAILED=1,
    STATUS_MISSING_FRAME=2,
    STATUS_FAILED_TO_SEND_FRAME=3
};


void set_status(enum LED_STATUS_BITS status);

static uint8_t framebuffer_static[640*480 / 5]={0};

QueueHandle_t uart_queue;

void send_frame(camera_fb_t *pic);

TaskHandle_t error_task;

void error_led_task(void *arg);

void init_uart();


void app_main(void)
{
    esp_log_level_set("*",ESP_LOG_INFO);
    esp_log_level_set("MAIN",ESP_LOG_DEBUG);
    esp_log_level_set("MOVIE",ESP_LOG_DEBUG);

    gpio_set_direction(LED_PIN,GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN,1);

    init_uart();

    xTaskCreate(error_led_task,"LED",4096,NULL,configMAX_PRIORITIES - 1,&error_task);
    vTaskSuspend(error_task);

    camera_config_t camera_config={
        .pin_pwdn=CAM_PWR_PIN,
        .pin_reset=GPIO_NUM_NC,
        .pin_xclk=CAM_MCLK,
        .pin_sccb_sda=I2C_CAM_SDA,
        .pin_sccb_scl=I2C_CAM_SCL,

        .pin_d7=CAM_D7,
        .pin_d6=CAM_D6,
        .pin_d5=CAM_D5,
        .pin_d4=CAM_D4,
        .pin_d3=CAM_D3,
        .pin_d2=CAM_D2,
        .pin_d1=CAM_D1,
        .pin_d0=CAM_D0,
        .pin_vsync=CAM_VSYNC,
        .pin_href=CAM_HSYNC,
        .pin_pclk=CAM_PCLK,

        .xclk_freq_hz=24000000,
        .ledc_timer=LEDC_TIMER_0,
        .ledc_channel=LEDC_CHANNEL_0,

        .pixel_format=PIXFORMAT_JPEG,
        .frame_size=FRAMESIZE_QVGA,
        
        .jpeg_quality=45,
        .fb_count=10,
        .fb_location=CAMERA_FB_IN_PSRAM,
        .grab_mode=CAMERA_GRAB_LATEST,

    };

    if(esp_camera_init(&camera_config)!=ESP_OK)
    {
        ESP_LOGE("MAIN","Failed to initialize camera");
        set_status(STATUS_CAMERA_INIT_FAILED);
    }
    else
    {
        set_status(STATUS_OK);   
    }

    gpio_set_direction(FLASH_LIGH,GPIO_MODE_OUTPUT);
    gpio_set_level(FLASH_LIGH,0);

    while(1)
    {
        TickType_t start=xTaskGetTickCount();
        // pobranie ostatniej przechwyconej klatki
        camera_fb_t *pic = esp_camera_fb_get();

        ESP_LOGI("MAIN", "Picture taken! Its size was: %zu bytes in time: %lu", pic->len,(xTaskGetTickCount()-start)*portTICK_PERIOD_MS);

        start=xTaskGetTickCount();
        // wyślij obraz przez port szeregowy
        send_frame(pic);
        // use pic->buf to access the image
        ESP_LOGI("MAIN", "Picture saved in time: %lu", (xTaskGetTickCount()-start)*portTICK_PERIOD_MS);

        // zwróci buffor na obraz
        esp_camera_fb_return(pic);

        
        taskYIELD();
    }

}



void init_uart()
{
    const size_t uart_buffer_size = ( sizeof(framebuffer_static) + HEADER_SIZE )*3;

    ESP_ERROR_CHECK(uart_driver_install(UART_ID, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));

    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_ID, &uart_config));

    ESP_ERROR_CHECK(uart_set_mode(UART_ID,UART_MODE_UART));
    // Set UART pins
    ESP_ERROR_CHECK(uart_set_pin(UART_ID, UART_TX_PIN, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC));
}


// dodaje nową ramkę do obecnie otwartego pliku
void send_frame(camera_fb_t *pic)
{
    if(pic == NULL)
    {
        ESP_LOGD("MOVIE","Got empty frame!");
        set_status(STATUS_MISSING_FRAME);
        return;
    }

    ESP_LOGD("MOVIE","Sending frame to UART");

    size_t frame_size = pic->len;

    uint32_t crc = crc32_le(0,pic->buf,frame_size);

    // frame size + crc
    uint8_t header[HEADER_SIZE];

    const char* header_start = MAGIC_WORD;

    header[0] = header_start[0];
    header[1] = header_start[1];

    // copy data to header
    memmove(&header[2],&frame_size,4);
    memmove(&header[6],&crc,4);

    // send header
    uart_write_bytes(UART_ID,(const char *)header,HEADER_SIZE);

    // send frame data
    uart_write_bytes(UART_ID,(const char *)pic->buf,frame_size);

    uart_flush(UART_ID);
}


volatile uint64_t error_timestamp = 1000;

void error_led_task(void* arg)
{
    bool error_stat = false;
    while(true)
    {
        gpio_set_level(LED_PIN,error_stat);

        error_stat=!error_stat;

        vTaskDelay(error_timestamp/portTICK_PERIOD_MS);
    }   
}


void set_status(enum LED_STATUS_BITS status)
{

    vTaskSuspend(error_task);

    switch (status)
    {
        case STATUS_CAMERA_INIT_FAILED:
            error_timestamp = 1000;
            vTaskResume(error_task);
        break;

        case STATUS_FAILED_TO_SEND_FRAME:
            error_timestamp = 2000;
            vTaskResume(error_task);
        break;

        case STATUS_MISSING_FRAME:
            error_timestamp = 50;
            vTaskResume(error_task);
        break;

        case STATUS_OK:
            gpio_set_level(LED_PIN,0);
        break;
    
        default:
            break;
    }

}