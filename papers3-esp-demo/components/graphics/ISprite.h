#pragma once

#include "IeSPI.h"
#include "Sprite.h"

// Legacy class for overriding driver code
class ISprite : public TFT_eSprite
{
private:
    IeSPI *_itft;
public:
    ISprite(IeSPI *tft);
    ~ISprite();

    void pushSprite(int32_t x, int32_t y, int32_t w, int32_t h, int mode = 0);
    void pushSprite(int32_t x, int32_t y, int mode = 0);
    void pushSprite(int mode = 0);
};

inline ISprite::ISprite(IeSPI *tft):TFT_eSprite(tft)
{
    _itft = tft;
}

inline ISprite::~ISprite()
{
}

inline void ISprite::pushSprite(int mode)
{
    _itft->updateDisplay(_dwidth * _dheight, (uint8_t*)_img, mode);
}

inline void ISprite::pushSprite(int32_t x, int32_t y, int mode)
{
    _itft->updateDisplay(x, y, _dwidth, _dheight, _img8, mode);
}

inline void ISprite::pushSprite(int32_t x, int32_t y, int32_t w, int32_t h, int mode)
{
    _itft->updateDisplay(x, y, w, h, _img8, mode);
}
