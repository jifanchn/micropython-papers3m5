#pragma once

#include "IeSPI.h"
#include "epd_highlevel.h"
#include "epdiy.h"
extern const EpdBoardDefinition epd_board_gtxyj;

// 原始framebuffer的分辨率
#define WIDTH1  540
#define HEIGHT1 960

// 新framebuffer的分辨率
#define WIDTH2  960
#define HEIGHT2 540

class ED047TC1Driver : public IeSPI {
private:
    EpdiyHighlevelState hl;

public:
    ED047TC1Driver(/* args */);
    ~ED047TC1Driver();

    void init(uint8_t tc = TAB_COLOUR);

    void updateDisplay(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t* data, int mode = MODE_GC16);
    void updateDisplay(int32_t size, uint8_t* data, int mode = MODE_GC16);
    // These are used to render images or sprites stored in RAM arrays (used by Sprite class for 16bpp Sprites)
    void setAddress(int32_t x, int32_t y);
    uint8_t* getFrameBuffer()
    {
        return epd_hl_get_framebuffer(&hl);
    }

    void clear()
    {
        epd_poweron();
        epd_clear();
        epd_poweroff();
    }
};

inline ED047TC1Driver::ED047TC1Driver() : IeSPI(540, 960)
{
}

inline ED047TC1Driver::~ED047TC1Driver()
{
}

inline void ED047TC1Driver::init(uint8_t tc)
{
    epd_init(&epd_board_gtxyj, &ED047TC2, EPD_LUT_64K);
    hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
}

inline void ED047TC1Driver::setAddress(int32_t x, int32_t y)
{
}

void rotateFramebufferMinus90_2x2(uint8_t* framebuffer1, uint8_t* framebuffer2)
{
    for (int y = 0; y < HEIGHT1; y += 2) {
        for (int x = 0; x < WIDTH1; x += 2) {
            // 计算原始2x2块的索引
            int index1_top    = (y * WIDTH1 + x) >> 1;
            int index1_bottom = ((y + 1) * WIDTH1 + x) >> 1;

            // 计算新的2x2块的索引
            int new_y_left   = WIDTH1 - x - 1;
            int new_y_right  = WIDTH1 - x - 2;
            int index2_left  = (new_y_left * WIDTH2 + y) >> 1;
            int index2_right = (new_y_right * WIDTH2 + y) >> 1;

            // 设置新的2x2块的像素值
            framebuffer2[index2_right] =
                ((framebuffer1[index1_top] & 0xF0) >> 4) | ((framebuffer1[index1_bottom] & 0xF0));
            framebuffer2[index2_left] =
                ((framebuffer1[index1_top] & 0x0F)) | ((framebuffer1[index1_bottom] & 0x0F) << 4);
        }
    }
}

void rotateFramebufferMinus90_2x2(uint8_t* framebuffer1, uint8_t* framebuffer2, int startX, int startY, int width,
                                  int height)
{
    int endY = startY + height;
    int endX = startX + width;
    for (int y = startY; y < endY; y += 2) {
        for (int x = startX; x < endX; x += 2) {
            // 计算原始2x2块的索引
            int index1_top    = (y * WIDTH1 + x) >> 1;
            int index1_bottom = ((y + 1) * WIDTH1 + x) >> 1;

            // 计算新的2x2块的索引
            int new_y_left   = WIDTH1 - x - 1;
            int new_y_right  = WIDTH1 - x - 2;
            int index2_left  = (new_y_left * WIDTH2 + y) >> 1;
            int index2_right = (new_y_right * WIDTH2 + y) >> 1;

            // 设置新的2x2块的像素值
            framebuffer2[index2_right] =
                ((framebuffer1[index1_top] & 0xF0) >> 4) | ((framebuffer1[index1_bottom] & 0xF0));
            framebuffer2[index2_left] = (framebuffer1[index1_top] & 0x0F) | ((framebuffer1[index1_bottom] & 0x0F) << 4);
        }
    }
}

inline void ED047TC1Driver::updateDisplay(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t* data, int mode)
{
    EpdRect area;
    area.x      = x;
    area.y      = y;
    area.width  = w;
    area.height = h;
    epd_poweron();
    epd_hl_update_area(&hl, (EpdDrawMode)mode, 10, area);
    epd_poweroff();
}

inline void ED047TC1Driver::updateDisplay(int32_t size, uint8_t* data, int mode)
{
    uint32_t t1 = micros();
    epd_poweron();
    epd_hl_update_screen(&hl, (EpdDrawMode)mode, 10);
    epd_poweroff();
}