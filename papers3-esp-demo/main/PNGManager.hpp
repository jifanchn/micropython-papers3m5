#pragma once

#include "esp_heap_caps.h"
#include <PNGdec.h>

class PNGManager {
private:
    uint8_t *m_buffer;
    uint32_t m_buffer_size;
    uint32_t m_used_size;

public:
    PNGManager(uint32_t buffer_size)
    {
        m_buffer_size = buffer_size;
    }

    ~PNGManager()
    {
        heap_caps_free(m_buffer);
    }

    void init()
    {
        m_buffer = (uint8_t *)heap_caps_malloc(m_buffer_size, MALLOC_CAP_SPIRAM);
        memset(m_buffer, 0, m_buffer_size);
        m_used_size = 0;
        png.setBuffer(m_buffer);
    }

    uint8_t *open(const uint8_t *png_src, uint32_t data_length);

    void clear();
};

extern PNGManager pngManager;