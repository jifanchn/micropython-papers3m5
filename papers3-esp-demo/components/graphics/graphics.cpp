#include "graphics.h"
#include "esp32-hal-log.h"
#include "esp_heap_caps.h"

ST7789Parallel g_st7789;
ISprite g_sprite(&g_st7789);

void graphics_init()
{
	g_st7789.init(0);
    g_sprite.setColorDepth(16);
    g_sprite.setSwapBytes(false);
    uint8_t * p_buffer = (uint8_t*)heap_caps_malloc(240 * 240 * 3 + 1, MALLOC_CAP_SPIRAM);
    if (p_buffer == NULL) {
        ESP_LOGE("Graphics", "Failed to allocate gram buffer");
    }
    g_sprite.createSprite(240, 240, 0, p_buffer);
    g_sprite.fillSprite(0);
}


void enter_err_state(const char* msg, uint8_t is_restart)
{
    ESP_LOGE("ERR", "%s", msg);
    g_sprite.fillSprite(TFT_RED);
    // g_sprite.useFreetypeFont(false);
    g_sprite.setTextFont(0);
    g_sprite.setTextColor(0xFFFF);
    g_sprite.setTextSize(2);
    g_sprite.setTextDatum(TL_DATUM);
    g_sprite.drawString(msg, 2, 2);
    // g_sprite.setCursor(0, 10);
    // g_sprite.printf("%s", msg);
    g_sprite.pushSprite(0, 0);

    vTaskDelay(1000 * 5); // 5s
    if (is_restart) {
        esp_restart();
    }
    // g_sprite.useFreetypeFont(true);
}
