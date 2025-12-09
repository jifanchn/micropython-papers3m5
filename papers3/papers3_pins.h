/*
 * Papers3 Pin Definitions
 * Centralized pin configuration to avoid conflicts
 */

#ifndef PAPERS3_PINS_H
#define PAPERS3_PINS_H

#include "driver/gpio.h"

// I2C Bus (Shared by Touch, RTC, Gyro/IMU)
#define PAPERS3_I2C_SDA     GPIO_NUM_41
#define PAPERS3_I2C_SCL     GPIO_NUM_42
#define PAPERS3_I2C_PORT    I2C_NUM_0

// Touch Screen (GT911)
#define PAPERS3_TOUCH_INT   GPIO_NUM_48
// Touch uses I2C pins

// SD Card (SPI)
// Corrected pins based on papers3-esp-demo
#define PAPERS3_SD_SCK      GPIO_NUM_39
#define PAPERS3_SD_MISO     GPIO_NUM_40
#define PAPERS3_SD_MOSI     GPIO_NUM_38
#define PAPERS3_SD_CS       GPIO_NUM_47

// E-Paper Display (EPD) - Data Bus
#define PAPERS3_EPD_D0      GPIO_NUM_6
#define PAPERS3_EPD_D1      GPIO_NUM_14
#define PAPERS3_EPD_D2      GPIO_NUM_7
#define PAPERS3_EPD_D3      GPIO_NUM_12
#define PAPERS3_EPD_D4      GPIO_NUM_9
#define PAPERS3_EPD_D5      GPIO_NUM_11
#define PAPERS3_EPD_D6      GPIO_NUM_8
#define PAPERS3_EPD_D7      GPIO_NUM_10

// E-Paper Display (EPD) - Control
#define PAPERS3_EPD_CKH     GPIO_NUM_16
#define PAPERS3_EPD_CKV     GPIO_NUM_18
#define PAPERS3_EPD_STH     GPIO_NUM_13
#define PAPERS3_EPD_SPV     GPIO_NUM_17 // Also STV
#define PAPERS3_EPD_XLE     GPIO_NUM_15 // Latch Enable
#define PAPERS3_EPD_EN      GPIO_NUM_45 // Power Enable
#define PAPERS3_EPD_BST     GPIO_NUM_46 // Boost Enable

// System / Other
#define PAPERS3_BAT_ADC     GPIO_NUM_3
#define PAPERS3_LED         GPIO_NUM_0
#define PAPERS3_BUZZER      GPIO_NUM_21
#define PAPERS3_USB_DET     GPIO_NUM_5
#define PAPERS3_CHG_STAT    GPIO_NUM_4

#endif // PAPERS3_PINS_H
