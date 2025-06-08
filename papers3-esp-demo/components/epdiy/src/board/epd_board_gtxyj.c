#include "epd_board.h"
#include "epdiy.h"
#include <stdint.h>

#include "esp_log.h"
#include "../output_lcd/lcd_driver.h"
#include "../output_common/render_method.h"

#include <sdkconfig.h>
#include <driver/gpio.h>
#include <driver/i2c.h>

// Make this compile von the ESP32 without ifdefing the whole file
#ifndef CONFIG_IDF_TARGET_ESP32S3
#define GPIO_NUM_40 -1
#define GPIO_NUM_41 -1
#define GPIO_NUM_42 -1
#define GPIO_NUM_43 -1
#define GPIO_NUM_44 -1
#define GPIO_NUM_45 -1
#define GPIO_NUM_46 -1
#define GPIO_NUM_47 -1
#define GPIO_NUM_48 -1
#endif


#define EPD_SPV GPIO_NUM_17
#define EPD_EN  GPIO_NUM_45
#define BST_EN  GPIO_NUM_46
#define EPD_XLE GPIO_NUM_15

/* Config Reggister Control */
// #define CFG_DATA GPIO_NUM_13*
// #define CFG_CLK GPIO_NUM_12*
// #define CFG_STR GPIO_NUM_0*

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

static lcd_bus_config_t lcd_config = {
    .clock = CKH,
    .ckv = CKV,
    .leh = EPD_XLE,
    .start_pulse = STH,
    .stv = EPD_SPV,
    .data_0 = D0,
    .data_1 = D1,
    .data_2 = D2,
    .data_3 = D3,
    .data_4 = D4,
    .data_5 = D5,
    .data_6 = D6,
    .data_7 = D7
};

inline static void fast_gpio_set_hi(gpio_num_t gpio_num)
{
    // GPIO.out_w1ts = (1 << gpio_num);
    gpio_set_level(gpio_num, 1);
}

inline static void fast_gpio_set_lo(gpio_num_t gpio_num)
{
    // GPIO.out_w1tc = (1 << gpio_num);
    gpio_set_level(gpio_num, 0);
}

void IRAM_ATTR busy_delay(uint32_t cycles)
{
    volatile uint64_t counts = XTHAL_GET_CCOUNT() + cycles;
    while (XTHAL_GET_CCOUNT() < counts) ;
}

static void epd_board_init(uint32_t epd_row_width) {
    gpio_hold_dis(CKH); // free CKH after wakeup

    gpio_set_direction(EPD_SPV, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_EN, GPIO_MODE_OUTPUT);
    gpio_set_direction(BST_EN, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_XLE, GPIO_MODE_OUTPUT);

    fast_gpio_set_lo(EPD_XLE);
    fast_gpio_set_hi(EPD_SPV);
    fast_gpio_set_lo(EPD_EN);
    fast_gpio_set_lo(BST_EN);

    const EpdDisplay_t* display = epd_get_display();

    LcdEpdConfig_t config = {
        .pixel_clock = display->bus_speed * 1000 * 1000,
          .ckv_high_time = 60,
        .line_front_porch = 4,
        .le_high_time = 4,
        .bus_width = display->bus_width,
        .bus = lcd_config,
    };
    epd_lcd_init(&config, display->width, display->height);
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
  // ESP_LOGW("epd", "Power on");
  // while (1)
  // {
  //   vTaskDelay(portMAX_DELAY);
  // }
  
}

static void epd_board_poweroff(epd_ctrl_state_t *state) {
  // fast_gpio_set_lo(BST_EN);
  // busy_delay(10 * 240);
  // fast_gpio_set_lo(EPD_EN);
  // busy_delay(100 * 240);
  fast_gpio_set_lo(EPD_SPV);
  // ESP_LOGW("epd", "Power off");
}

static float epd_board_ambient_temperature() {
  return 25;
}

static void set_vcom(int value) {

}

const EpdBoardDefinition epd_board_gtxyj = {
  .init = epd_board_init,
  .deinit = epd_board_deinit,
  .set_ctrl = epd_board_set_ctrl,
  .poweron = epd_board_poweron,
  .poweroff = epd_board_poweroff,

  .get_temperature = epd_board_ambient_temperature,
  .set_vcom = set_vcom,

  // unimplemented for now, but shares v6 implementation
  .gpio_set_direction = NULL,
  .gpio_read = NULL,
  .gpio_write = NULL,
};

