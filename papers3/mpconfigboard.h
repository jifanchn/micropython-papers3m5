#define MICROPY_HW_BOARD_NAME               "Papers3M5 ESP32S3-N16R8"
#define MICROPY_HW_MCU_NAME                 "ESP32S3"

// Enable UART REPL for modules that have an external USB-UART and don't use native USB.
#define MICROPY_HW_ENABLE_UART_REPL         (1)

// I2C configuration (based on papers3-esp-demo hardware)
#define MICROPY_HW_I2C0_SCL                 (9)
#define MICROPY_HW_I2C0_SDA                 (8)

// SPI configuration for EPD
#define MICROPY_HW_SPI1_SCK                 (18)
#define MICROPY_HW_SPI1_MOSI                (23)
#define MICROPY_HW_SPI1_MISO                (19)

// Papers3 EPD Pin Definitions (from epd_board_gtxyj.c)
// Control Pins
#define MICROPY_HW_EPD_SPV                  (17)  // Start Pulse Vertical
#define MICROPY_HW_EPD_EN                   (45)  // EPD Enable  
#define MICROPY_HW_EPD_BST_EN               (46)  // Boost Enable
#define MICROPY_HW_EPD_XLE                  (15)  // Latch Enable
#define MICROPY_HW_EPD_CKV                  (18)  // Clock Vertical
#define MICROPY_HW_EPD_STH                  (13)  // Start Pulse Horizontal
#define MICROPY_HW_EPD_CKH                  (16)  // Clock Horizontal

// Data Lines D0-D7 (8-bit parallel bus)
#define MICROPY_HW_EPD_D0                   (6)
#define MICROPY_HW_EPD_D1                   (14)
#define MICROPY_HW_EPD_D2                   (7)
#define MICROPY_HW_EPD_D3                   (12)
#define MICROPY_HW_EPD_D4                   (9)
#define MICROPY_HW_EPD_D5                   (11)
#define MICROPY_HW_EPD_D6                   (8)
#define MICROPY_HW_EPD_D7                   (10) 