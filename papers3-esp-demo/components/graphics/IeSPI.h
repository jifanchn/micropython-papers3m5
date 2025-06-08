#pragma once

#include <SPI.h>
#include "TFT_eSPI.h"

// Legacy class for overriding driver code
class IeSPI: public TFT_eSPI 
{
private:
    /* data */
public:
    IeSPI(int w, int h);
    ~IeSPI();
    virtual void updateDisplay(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t* data, int mode);
    virtual void updateDisplay(int32_t size, uint8_t* data, int mode);
};

inline IeSPI::IeSPI(int w, int h): TFT_eSPI(w, h)
{
}

inline IeSPI::~IeSPI()
{
}
