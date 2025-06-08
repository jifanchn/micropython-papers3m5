/*
 * Papers3 Board Definition for EPDiy
 * Based on successful epd_board_gtxyj implementation from papers3-esp-demo
 */

#include "epd_board.h"
#include "epdiy.h"
#include <stdint.h>

#include "esp_log.h"
#include "../output_common/render_method.h"

#include <sdkconfig.h>
#include <driver/gpio.h>
#include <driver/i2c.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>

// GPIO pin definitions for Papers3 board (same as gtxyj)
#define EPD_SPV GPIO_NUM_17
#define EPD_EN  GPIO_NUM_45
#define BST_EN  GPIO_NUM_46
#define EPD_XLE GPIO_NUM_15

/* Control Lines */
#define CKV GPIO_NUM_18
#define STH GPIO_NUM_13

/* Edges */
#define CKH GPIO_NUM_16

/* Data Lines */
#define D7 GPIO_NUM_10
#define D6 GPIO_NUM_8
#define D5 GPIO_NUM_11
#define D4 GPIO_NUM_9
#define D3 GPIO_NUM_12
#define D2 GPIO_NUM_7
#define D1 GPIO_NUM_14
#define D0 GPIO_NUM_6

inline static void fast_gpio_set_hi(gpio_num_t gpio_num)
{
    gpio_set_level(gpio_num, 1);
}

inline static void fast_gpio_set_lo(gpio_num_t gpio_num)
{
    gpio_set_level(gpio_num, 0);
}

void IRAM_ATTR busy_delay(uint32_t cycles)
{
    volatile uint64_t counts = xthal_get_ccount() + cycles;
    while (xthal_get_ccount() < counts) ;
}

static void epd_board_init(uint32_t epd_row_width) {
    ESP_LOGI("Papers3", "EPD board init, row width: %" PRIu32, epd_row_width);
    
    gpio_hold_dis(CKH); // free CKH after wakeup

    gpio_set_direction(EPD_SPV, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_EN, GPIO_MODE_OUTPUT);
    gpio_set_direction(BST_EN, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_XLE, GPIO_MODE_OUTPUT);

    fast_gpio_set_lo(EPD_XLE);
    fast_gpio_set_hi(EPD_SPV);
    fast_gpio_set_lo(EPD_EN);
    fast_gpio_set_lo(BST_EN);
}

static void epd_board_deinit() {
    fast_gpio_set_lo(EPD_XLE);
    fast_gpio_set_lo(EPD_SPV);
    fast_gpio_set_lo(EPD_EN);
    fast_gpio_set_lo(BST_EN);
}

static void epd_board_set_ctrl(epd_ctrl_state_t *state, const epd_ctrl_state_t * const mask) {
    if (state->ep_sth) {
        fast_gpio_set_hi(STH);
    } else {
        fast_gpio_set_lo(STH);
    }

    if (state->ep_stv) {
        fast_gpio_set_hi(EPD_SPV);
    } else {
        fast_gpio_set_lo(EPD_SPV);
    }

    if (state->ep_latch_enable) {
        fast_gpio_set_hi(EPD_XLE);
        fast_gpio_set_hi(EPD_XLE);
    } else {
        fast_gpio_set_lo(EPD_XLE);
        fast_gpio_set_lo(EPD_XLE);
    }
}

static void epd_board_poweron(epd_ctrl_state_t *state) {
    fast_gpio_set_hi(EPD_EN);
    busy_delay(100 * 240);
    fast_gpio_set_hi(BST_EN);
    busy_delay(100 * 240);
    fast_gpio_set_hi(EPD_SPV);
    fast_gpio_set_hi(STH);
}

static void epd_board_poweroff(epd_ctrl_state_t *state) {
    fast_gpio_set_lo(EPD_SPV);
}

static float epd_board_ambient_temperature() {
    return 25.0f;
}

static void set_vcom(int value) {
    // No VCOM control needed for this board
}

const EpdBoardDefinition epd_board_papers3 = {
    .init = epd_board_init,
    .deinit = epd_board_deinit,
    .set_ctrl = epd_board_set_ctrl,
    .poweron = epd_board_poweron,
    .poweroff = epd_board_poweroff,
    .get_temperature = epd_board_ambient_temperature,
    .set_vcom = set_vcom,
    
    // unimplemented for now
    .gpio_set_direction = NULL,
    .gpio_read = NULL,
    .gpio_write = NULL,
}; 