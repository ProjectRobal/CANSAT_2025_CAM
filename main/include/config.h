#pragma once

#include <driver/i2c.h>
#include <driver/uart.h>
#include <driver/gpio.h>

#define SD_MOUNT_POINT "/sdcard"

#define CHECKPOINT_FILE "/sdcard/checkpoint"

#define MAGIC_WORD "VW"

#define HEADER_SIZE 10

#define UART_ID UART_NUM_2

#define UART_TX_PIN GPIO_NUM_4

#define UART_BAUD_RATE 115200


#define OV2640_ADDRESS 0x30

#define OV2640_I2C_INTERFACE I2C_NUM_0

#define LED_PIN GPIO_NUM_33


// leds gpio

#define SIGNAL_LED GPIO_NUM_33

#define FLASH_LIGH GPIO_NUM_4

// i2c port configuration

#define I2C_MASTER_FREQ_HZ 100000

#define I2C_CAM_PORT I2C_NUM_0

#define I2C_CAM_SDA GPIO_NUM_26

#define I2C_CAM_SCL GPIO_NUM_27

#define SD_CARD_ERROR_TIME 500000

#define CAM_ERROR_TIME 1000000

#define BOTH_ERROR_TIME 50000

// cam related gpio

#define CAM_PWR_PIN GPIO_NUM_32

// I2S CSI pins

#define CAM_D0 GPIO_NUM_5
#define CAM_D1 GPIO_NUM_18
#define CAM_D2 GPIO_NUM_19
#define CAM_D3 GPIO_NUM_21
#define CAM_D4 GPIO_NUM_36
#define CAM_D5 GPIO_NUM_39
#define CAM_D6 GPIO_NUM_34
#define CAM_D7 GPIO_NUM_35

#define CAM_HSYNC GPIO_NUM_23
#define CAM_VSYNC GPIO_NUM_25

#define CAM_PCLK GPIO_NUM_22

#define CAM_MCLK GPIO_NUM_0

//#define CAM_RST_PIN GPIO_NUM_

//#define CAM_PIN_XCLK GPIO_NUM_25