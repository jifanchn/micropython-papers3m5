#ifndef _BOARD_H_
#define _BOARD_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "board_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define PROB  ESP_LOGW("PROB", "_prob_ %d", __LINE__);
#define READ(x, type, p_buffer) x = *((type*)(p_buffer))
#define SAFE_FREE(p)  if (p != NULL) {free(p);}

#define EMBED_FILES_DECLARE(file_name) \
extern uint8_t _binary_##file_name##_start; \
extern uint8_t _binary_##file_name##_end; \
static const uint8_t* _##file_name##_ptr = &_binary_##file_name##_start; \
static uint32_t _##file_name##_size = (uint32_t)&_binary_##file_name##_end - (uint32_t)&_binary_##file_name##_start

static inline int64_t m_millis()
{
    return esp_timer_get_time() / 1000;
}

static inline int64_t m_micros()
{
    return esp_timer_get_time();
}

#define CHECK_RAM_USAGE(tag)  ESP_LOGI(TAG, "%s: Free IRAM: %d bytes, PRAM: %d bytes", tag, esp_get_free_internal_heap_size(), esp_get_free_heap_size())

int getRandInt(int start, int end);

#ifdef __cplusplus
}
#endif
#endif