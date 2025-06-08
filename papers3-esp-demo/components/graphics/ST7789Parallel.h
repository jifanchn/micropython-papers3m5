#pragma once

#include "IeSPI.h"

class ST7789Parallel : public IeSPI {
   private:

   public:
    ST7789Parallel(/* args */);
    ~ST7789Parallel();

    void init(uint8_t tc = TAB_COLOUR);

    void updateDisplay(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t* data);
    void updateDisplay(int32_t size, uint8_t* data);
    // These are used to render images or sprites stored in RAM arrays (used by Sprite class for 16bpp Sprites)
    void setAddress(int32_t x, int32_t y);

};

inline ST7789Parallel::ST7789Parallel() : IeSPI(240, 240) 
{

}

inline ST7789Parallel::~ST7789Parallel() {}

inline void ST7789Parallel::init(uint8_t tc) {
    // Screen_Init(&m_lcd, NULL);
}

inline void ST7789Parallel::setAddress(int32_t x, int32_t y) {
    
}

inline void ST7789Parallel::updateDisplay(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t* data) 
{
    // m_lcd.draw_bitmap(x, y, w, h, (uint16_t *)data);
}

inline void ST7789Parallel::updateDisplay(int32_t size, uint8_t* data)
{
    // m_lcd.draw_bitmap(0, 0, 240, 240, (uint16_t *)data);
}