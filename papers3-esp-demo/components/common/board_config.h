#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#define MAIN_VERSION    2
#define SUB_VERSION     1
#define IS_PRO_VERSION  1
// #define BOARD_HAS_AW8898
// #define DEVELOP_BOARD

// touchpad
#define PIN_TP0  (gpio_num_t)10
#define PIN_TP1  (gpio_num_t)11

// BT module
#define PIN_MUTE (gpio_num_t)21
#define PLAYING_LEVEL   1
#define STOPPED_LEVEL   (!PLAYING_LEVEL)

// SD Card
#ifdef DEVELOP_BOARD
    #define PIN_SD_MOSI             41
    #define PIN_SD_MISO             42
    #define PIN_SD_CLK              40
    #define PIN_SD_CS               39
#else
    #define PIN_SD_MOSI             1
    #define PIN_SD_MISO             42
    #define PIN_SD_CLK              2
    #define PIN_SD_CS               41
#endif

// Speaker
#define SPEAKER_SAMPLE_RATE     44100
#define SPEAKER_SAMPLE_RATE_KHZ 44.1f
#define SPEAKER_I2S_DEVICE_NUM  (i2s_port_t)1
#define AW8898_ADDRESS          0x34
#define PIN_AW8898_DIN          16
#define PIN_AW8898_BCK          17
#define PIN_AW8898_WCK          18
#define PIN_AW8898_SDA          8
#define PIN_AW8898_SCL          9

#define PIN_NS4168_CTRL         45
#define PIN_NS4168_BCLK         12
#define PIN_NS4168_LRCLK        13
#define PIN_NS4168_SDATA        14

// MIC
#define MIC_SAMPLE_RATE         16000
#define MIC_SAMPLE_RATE_KHZ     16.0f
#define MIC_I2S_DEVICE_NUM      0
#ifdef DEVELOP_BOARD
    #define PIN_MIC_PDM_CLK         21
    #define PIN_MIC_PDM_DATA        20
#else
    #define PIN_MIC_PDM_CLK         19
    #define PIN_MIC_PDM_DATA        20
#endif

// LCD
// #define BOARD_LCD_INTERFACE_SPI 
#ifndef BOARD_LCD_INTERFACE_SPI
    #define BOARD_LCD_INTERFACE_MCU 
#endif

#ifdef DEVELOP_BOARD
    #define PIN_LCD_BCKL            -1
#else
    #define PIN_LCD_BCKL            9
#endif

#ifdef BOARD_LCD_INTERFACE_SPI
    #define PIN_LCD_SPI_MOSI        7
    #define PIN_LCD_SPI_CLK         6
    #define PIN_LCD_SPI_DC          4
    #define PIN_LCD_SPI_CS          5
    #define PIN_LCD_SPI_RST         15
#else
    #ifdef DEVELOP_BOARD
        #define PIN_LCD_MCU_D0          10
        #define PIN_LCD_MCU_D1          46
        #define PIN_LCD_MCU_D2          3
        #define PIN_LCD_MCU_D3          38
        #define PIN_LCD_MCU_D4          7
        #define PIN_LCD_MCU_D5          6
        #define PIN_LCD_MCU_D6          5
        #define PIN_LCD_MCU_D7          4
        #define PIN_LCD_MCU_CS          14
        #define PIN_LCD_MCU_WR          12
        #define PIN_LCD_MCU_RS          13
        #define PIN_LCD_MCU_RD          11
        #define PIN_LCD_MCU_RST         2
        #define PIN_LCD_MCU_BL          -1
        #define PIN_LCD_MCU_TE          1
    #else
        #define PIN_LCD_MCU_D0          15
        #define PIN_LCD_MCU_D1          16
        #define PIN_LCD_MCU_D2          17
        #define PIN_LCD_MCU_D3          18
        #define PIN_LCD_MCU_D4          4
        #define PIN_LCD_MCU_D5          5
        #define PIN_LCD_MCU_D6          6
        #define PIN_LCD_MCU_D7          7
        #define PIN_LCD_MCU_CS          -1
        #define PIN_LCD_MCU_WR          3
        #define PIN_LCD_MCU_RS          8
        #define PIN_LCD_MCU_RD          46
        #define PIN_LCD_MCU_RST         -1
        #define PIN_LCD_MCU_BL          -1
        #define PIN_LCD_MCU_TE          -1
    #endif
#endif

#endif