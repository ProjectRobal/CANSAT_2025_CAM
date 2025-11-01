# CANSAT 2025 Camera

A simple project in which ESP32-CAM with OVA2640 reads JPEG encoded frames from camera
and send them through UART.

## Message format:

```
<header_start:2B><data_length:4B><crc_32:4B><data:"data_length"B>
```

where:
 - header_start - a two bytes word **"VW"**
 - data_length - an unsigned 32 bits integear value that indicates how much data to read
 - crc_32 - CRC32 checksum to check data integrity
 - data - a JPEG encoded frame from camera