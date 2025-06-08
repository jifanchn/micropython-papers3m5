#pragma once

#include <LovyanGFX.hpp>
#include "epd_highlevel.h"
#include "epdiy.h"

namespace lgfx {

class LGFXPaperS3 : public lgfx::Panel_Device {
private:
    EpdiyHighlevelState hl;
    // implement abstract methods
    void beginTransaction(void)
    {
    }
    void endTransaction(void)
    {
    }
    color_depth_t setColorDepth(color_depth_t depth)
    {
        return (color_depth_t)0;
    }
    void setInvert(bool invert)
    {
    }
    void setRotation(uint_fast8_t r)
    {
    }
    void setSleep(bool flg)
    {
    }
    void setPowerSave(bool flg)
    {
    }
    void waitDisplay(void)
    {
    }
    bool displayBusy(void)
    {
        return false;
    }
    void writeBlock(uint32_t rawcolor, uint32_t len)
    {
    }
    void drawPixelPreclipped(uint_fast16_t x, uint_fast16_t y, uint32_t rawcolor)
    {
    }
    void writeFillRectPreclipped(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, uint32_t rawcolor)
    {
    }
    void writePixels(pixelcopy_t* param, uint32_t len, bool use_dma)
    {
    }
    uint32_t readCommand(uint_fast16_t cmd, uint_fast8_t index, uint_fast8_t len)
    {
        return 0;
    }
    uint32_t readData(uint_fast8_t index, uint_fast8_t len)
    {
        return 0;
    }
    void readRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, void* dst, pixelcopy_t* param)
    {
    }

public:
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

    void updateDisplay(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t* data, int mode)
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

    void updateDisplay(int32_t size, uint8_t* data, int mode)
    {
        uint32_t t1 = micros();

        epd_poweron();
        epd_hl_update_screen(&hl, (EpdDrawMode)mode, 10);
        epd_poweroff();
    }

    bool init(bool use_reset) override
    {
        _write_bits  = 4;
        _width       = 960;
        _height      = 540;
        _write_depth = color_depth_t::grayscale_4bit;
        _read_depth  = color_depth_t::grayscale_4bit;
        _rotation    = 0;

        epd_init(&epd_board_gtxyj, &ED047TC2, EPD_LUT_64K);
        hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
        return true;
    }

    void setWindow(uint_fast16_t xs, uint_fast16_t ys, uint_fast16_t xe, uint_fast16_t ye) override
    {
    }

    void writeImage(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, pixelcopy_t* param,
                    bool use_dma)
    {
        auto bytes = param->dst_bits >> 3;
        auto src_x = param->src_x;
        uint32_t i = (src_x + param->src_y * param->src_bitwidth) * bytes;
        auto src   = &((const uint8_t*)param->src_data)[i];

        // printf("update");

        epd_clear();

        epd_poweron();
        epd_hl_update_screen(&hl, (EpdDrawMode)MODE_GC16, 10);
        epd_poweroff();
    }
};

}  // namespace lgfx
