#pragma once

#include <SPI.h>
#include "IeSPI.h"

class SH1122 : public IeSPI {
   private:
    SPIClass spi = SPIClass(VSPI);
    enum
    {
        SH1122_CS = 3,
        SH1122_DC = 22,
        SH1122_RST = 23,
        SH1122_MOSI = 2,
        SH1122_SCLK = 5,
        SH1122_SPI_FREQ = 40000000
    };
   public:
    SH1122(/* args */);
    ~SH1122();

    void init(uint8_t tc = TAB_COLOUR);
    void writeReg(uint8_t reg);
    void writeData(uint8_t *data, uint32_t len);
    void softWrite(uint8_t data);
    void fill(uint8_t);

    void updateDisplay(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t* data);
    void updateDisplay(int32_t size, uint8_t* data);
    // These are used to render images or sprites stored in RAM arrays (used by Sprite class for 16bpp Sprites)
    void setAddress(int32_t x, int32_t y);
};

inline SH1122::SH1122(/* args */) : IeSPI(256, 64) 
{

}

inline SH1122::~SH1122() {}

inline void SH1122::softWrite(uint8_t data)
{
    digitalWrite(SH1122_CS, 0);

    for (uint8_t i = 0; i < 8; i++)
    {
        gpio_set_level((gpio_num_t)SH1122_SCLK, 0);
        if (data & 0x80)
        {
            gpio_set_level((gpio_num_t)SH1122_MOSI, 1);
        }
        else
        {
            gpio_set_level((gpio_num_t)SH1122_MOSI, 0);
        }
        gpio_set_level((gpio_num_t)SH1122_SCLK, 1);
        data <<= 1;
    }

    digitalWrite(SH1122_CS, 1);
}

inline void SH1122::writeReg(uint8_t reg)
{
    digitalWrite(SH1122_DC, 0);
    // softWrite(reg);
    spi.beginTransaction(SPISettings(SH1122_SPI_FREQ, SPI_MSBFIRST, SPI_MODE0));
    spi.write(reg);
    spi.endTransaction();
    digitalWrite(SH1122_DC, 1);
}

inline void SH1122::writeData(uint8_t *data, uint32_t len)
{
    // for (int i = 0; i < len; i++)
    // {
    //     softWrite(data[i]);
    // }
    spi.beginTransaction(SPISettings(SH1122_SPI_FREQ, SPI_MSBFIRST, SPI_MODE0));
    spi.writeBytes(data, len);
    spi.endTransaction();
}

inline void SH1122::fill(uint8_t val)
{
    uint8_t data[128];
    memset(data, val, 128);

    spi.beginTransaction(SPISettings(SH1122_SPI_FREQ, SPI_MSBFIRST, SPI_MODE0));
    for (int i = 0; i < 64; i++)
    {
        digitalWrite(SH1122_DC, 0);
        spi.write(0xB0);
        spi.write(i);
        spi.write(0x10);
        spi.write(0);
        digitalWrite(SH1122_DC, 1);
        spi.writeBytes(data, 128);
    }
    spi.endTransaction();
}

inline void SH1122::init(uint8_t tc) {
    pinMode(SH1122_CS, OUTPUT);
    pinMode(SH1122_DC, OUTPUT);
    pinMode(SH1122_RST, OUTPUT);
    pinMode(SH1122_SCLK, OUTPUT);
    pinMode(SH1122_MOSI, PULLUP);

    spi.begin(SH1122_SCLK, 19, SH1122_MOSI, SH1122_CS);
    // spi.setFrequency(2000000);

    digitalWrite(SH1122_DC, 1);
    digitalWrite(SH1122_CS, 0);
    digitalWrite(SH1122_RST, 0);
    delay(5);
    digitalWrite(SH1122_RST, 1);
    delay(10);

    writeReg(0xAE);  // Set display off
    writeReg(0xB0);  // Row address Mode Setting
    writeReg(0x00);
    writeReg(0x10);  // Set Higher Column Address of display RAM
    writeReg(0x00);  // Set Lower Column Address of display RAM
    writeReg(0xD5);  // Set Display Clock Divide Ratio/Oscillator Frequency
    writeReg(0x50);  // 50 125hz
    writeReg(0xD9);  // Set Discharge/Precharge Period
    writeReg(0x22);
    writeReg(0x40);  // Set Display Start Line
    writeReg(0x81);  // The Contrast Control Mode Set
    writeReg(0xFF);
    // Horizontal
    writeReg(0xA1);  // Set Segment Re-map
    writeReg(0xC8);  // Set Common Output Scan Direction
    writeReg(0xD3);  // Set Display Offset
    writeReg(0x20);
    // Vertical
    // OLED_WR_REG(0xA0); //Set Segment Re-map
    // OLED_WR_REG(0xC0); //Set Common Output Scan Direction
    // OLED_WR_REG(0xD3); //Set Display Offset
    // OLED_WR_REG(0x00);
    writeReg(0xA4);  // Set Entire Display OFF/ON
    writeReg(0xA6);  // Set Normal/Reverse Display
    writeReg(0xA8);  // Set Multiplex Ration
    writeReg(0x3F);
    writeReg(0xAD);  // DC-DC Setting
    writeReg(0x80);  // DC-DC is disable
    writeReg(0xDB);  // Set VCOM Deselect Level
    writeReg(0x30);
    writeReg(0xDC);  // Set VSEGM Level
    writeReg(0x30);
    writeReg(0x33);  // Set Discharge VSL Level 1.8V
    fill(0);
    writeReg(0xAF);  // Set Display On
}

inline void SH1122::setAddress(int32_t x, int32_t y) {
    writeReg(0xB0);
    writeReg(y);
    writeReg(((x & 0xf0) >> 4) | 0x10);
    writeReg((x & 0x0f));
}

inline void SH1122::updateDisplay(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t* data) 
{
    w = w / 2 + ((w % 2) ? 1 : 0);
    x /= 2;

    spi.beginTransaction(SPISettings(SH1122_SPI_FREQ, SPI_MSBFIRST, SPI_MODE0));
    for (int i = 0; i < h; i++)
    {
        digitalWrite(SH1122_DC, 0);
        spi.write(0xB0);
        spi.write(y + i);
        spi.write(((x & 0xf0) >> 4) | 0x10);
        spi.write((x & 0x0f));
        digitalWrite(SH1122_DC, 1);
        spi.writeBytes(data, w);
    }
    spi.endTransaction();
}

inline void SH1122::updateDisplay(int32_t size, uint8_t* data)
{
    spi.beginTransaction(SPISettings(SH1122_SPI_FREQ, SPI_MSBFIRST, SPI_MODE0));
    digitalWrite(SH1122_DC, 0);
    spi.write(0xB0);
    spi.write(0);
    spi.write(0x10);
    spi.write(0);
    digitalWrite(SH1122_DC, 1);
    spi.writeBytes(data, size / 2);

    spi.endTransaction();
}