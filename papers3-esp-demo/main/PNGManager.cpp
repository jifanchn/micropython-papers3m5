#include "PNGManager.hpp"

#define TAG "PNGManager"

uint8_t *PNGManager::open(const uint8_t *png_src, uint32_t data_length)
{
    png.openFLASH((uint8_t *)png_src, data_length, NULL);
    uint32_t img_size = png.getWidth() * png.getHeight() * 3;
    // ESP_LOGI(TAG, "Image width: %d, height: %d, alpha: %d", png.getWidth(), png.getHeight(), png.hasAlpha());
    if (img_size > (m_buffer_size - m_used_size)) {
        ESP_LOGE(TAG, "Not enough buffer for image, need %d, left %d", img_size, m_buffer_size - m_used_size);
        return NULL;
    }
    uint8_t *buffer = m_buffer + m_used_size;
    png.setBuffer(buffer);
    png.decode(NULL, PNG_FAST_PALETTE);
    png.close();
    m_used_size += img_size;
    return buffer;
}

void PNGManager::clear()
{
    m_used_size = 0;
}

PNGManager pngManager(4 * 1024 * 1024);