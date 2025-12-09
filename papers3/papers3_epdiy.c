/*
 * Papers3 EPDiy Module Implementation
 * Based on demo project ED047TC1Driver.h
 * Refactored to use centralized pins and dynamic temperature
 */

#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "py/objstr.h"
#include "py/objint.h"

// ESP-IDF 
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// GPIO
#include "driver/gpio.h"

// EPDiy High Level API
#include "epdiy.h"
#include "epd_highlevel.h"
#include "epd_board.h"
#include "lcd_driver.h"
#include <inttypes.h>

#include "chinese_24.h"
#include "papers3_pins.h"

#ifndef STATIC
#define STATIC static
#endif

#define TAG "papers3_epdiy"

#define PAPERS3_WIDTH  960
#define PAPERS3_HEIGHT 540

// Pin Mapping from papers3_pins.h
#define EPD_SPV PAPERS3_EPD_SPV
#define EPD_EN  PAPERS3_EPD_EN
#define BST_EN  PAPERS3_EPD_BST
#define EPD_XLE PAPERS3_EPD_XLE
#define CKV     PAPERS3_EPD_CKV
#define STH     PAPERS3_EPD_STH
#define CKH     PAPERS3_EPD_CKH

// Data Bus
#define D7 PAPERS3_EPD_D7
#define D6 PAPERS3_EPD_D6
#define D5 PAPERS3_EPD_D5
#define D4 PAPERS3_EPD_D4
#define D3 PAPERS3_EPD_D3
#define D2 PAPERS3_EPD_D2
#define D1 PAPERS3_EPD_D1
#define D0 PAPERS3_EPD_D0

// LCD Bus Config
static lcd_bus_config_t papers3_lcd_config = {
    .clock = CKH,
    .ckv = CKV,
    .leh = EPD_XLE,
    .start_pulse = STH,
    .stv = EPD_SPV,
    .data = {
        [0] = D0,
        [1] = D1,
        [2] = D2,
        [3] = D3,
        [4] = D4,
        [5] = D5,
        [6] = D6,
        [7] = D7,
    }
};

// Global Temperature State
static float s_epd_temperature = 25.0f;

// Fast GPIO Utils
inline static void fast_gpio_set_hi(gpio_num_t gpio_num) {
    gpio_set_level(gpio_num, 1);
}

inline static void fast_gpio_set_lo(gpio_num_t gpio_num) {
    gpio_set_level(gpio_num, 0);
}

void IRAM_ATTR busy_delay(uint32_t cycles) {
    volatile uint64_t counts = esp_timer_get_time() + cycles;
    while (esp_timer_get_time() < counts) ;
}

// Board Interface
static void papers3_board_init(uint32_t epd_row_width) {
    ESP_LOGI(TAG, "Papers3 board init, row width: %"PRIu32, epd_row_width);
    
    gpio_hold_dis(CKH);
    
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
        .bus = papers3_lcd_config,
    };
    epd_lcd_init(&config, display->width, display->height);
}

static void papers3_board_deinit(void) {
    ESP_LOGI(TAG, "Papers3 board deinit");
    epd_lcd_deinit();
    
    fast_gpio_set_lo(EPD_XLE);
    fast_gpio_set_lo(EPD_SPV);
    fast_gpio_set_lo(EPD_EN);
    fast_gpio_set_lo(BST_EN);
}

static void papers3_board_set_ctrl(epd_ctrl_state_t* state, const epd_ctrl_state_t* const target) {
    if (!state || !target) return;
    
    if (target->ep_sth) fast_gpio_set_hi(STH);
    else fast_gpio_set_lo(STH);

    if (target->ep_stv) fast_gpio_set_hi(EPD_SPV);
    else fast_gpio_set_lo(EPD_SPV);

    if (target->ep_latch_enable) {
        fast_gpio_set_hi(EPD_XLE);
        fast_gpio_set_hi(EPD_XLE);
    } else {
        fast_gpio_set_lo(EPD_XLE);
        fast_gpio_set_lo(EPD_XLE);
    }
    
    *state = *target;
}

static void papers3_board_poweron(epd_ctrl_state_t* state) {
    ESP_LOGI(TAG, "Papers3 board power on");
    fast_gpio_set_hi(EPD_EN);
    busy_delay(100 * 240);
    fast_gpio_set_hi(BST_EN);
    busy_delay(100 * 240);
    fast_gpio_set_hi(EPD_SPV);
    fast_gpio_set_hi(STH);
}

static void papers3_board_measure_vcom(epd_ctrl_state_t* state) {
    // Not implemented
}

static void papers3_board_poweroff(epd_ctrl_state_t* state) {
    ESP_LOGI(TAG, "Papers3 board power off");
    fast_gpio_set_lo(EPD_SPV);
}

static void papers3_board_set_vcom(int vcom_mv) {
    // Not implemented
}

static float papers3_board_get_temperature(void) {
    return s_epd_temperature;
}

static esp_err_t papers3_board_gpio_set_direction(int pin, bool make_input) {
    return ESP_OK;
}

static bool papers3_board_gpio_read(int pin) {
    return false;
}

static esp_err_t papers3_board_gpio_write(int pin, bool value) {
    return ESP_OK;
}

static const EpdBoardDefinition papers3_board = {
    .init = papers3_board_init,
    .deinit = papers3_board_deinit,
    .set_ctrl = papers3_board_set_ctrl,
    .poweron = papers3_board_poweron,
    .measure_vcom = papers3_board_measure_vcom,
    .poweroff = papers3_board_poweroff,
    .set_vcom = papers3_board_set_vcom,
    .get_temperature = papers3_board_get_temperature,
    .gpio_set_direction = papers3_board_gpio_set_direction,
    .gpio_read = papers3_board_gpio_read,
    .gpio_write = papers3_board_gpio_write,
};

// Python Object
typedef struct _papers3_epdiy_obj_t {
    mp_obj_base_t base;
    EpdiyHighlevelState hl;
    bool initialized;
} papers3_epdiy_obj_t;

// Global flag to track if EPDiy library is initialized
// This prevents double-initialization across multiple objects or re-imports
static bool s_epdiy_global_initialized = false;

extern const mp_obj_type_t papers3_epdiy_type;

STATIC mp_obj_t papers3_epdiy_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    papers3_epdiy_obj_t *self = m_new_obj(papers3_epdiy_obj_t);
    self->base.type = &papers3_epdiy_type;
    self->initialized = false;
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t papers3_epdiy_init(mp_obj_t self_in) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        ESP_LOGW(TAG, "EPDiy object already initialized");
        return mp_const_none;
    }
    
    // Always initialize the driver to ensure correct hardware state
    // The previous crash was due to highlevel.c assertion, which is now fixed.
    ESP_LOGI(TAG, "Initializing EPDiy driver...");
    epd_init(&papers3_board, &ED047TC2, EPD_LUT_64K);
    s_epdiy_global_initialized = true;
    
    // Always initialize new high-level state (allocates framebuffers)
    self->hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
    
    self->initialized = true;
    ESP_LOGI(TAG, "EPDiy initialized");
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_deinit(mp_obj_t self_in) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        // Use the library function to clean up high-level state
        epd_hl_deinit(&self->hl);

        // Deinitialize the low-level driver
        epd_deinit();
        s_epdiy_global_initialized = false;
        
        self->initialized = false;
        ESP_LOGI(TAG, "EPDiy deinitialized");
    }
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_get_framebuffer(mp_obj_t self_in) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->initialized) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Not initialized"));
    
    uint8_t* framebuffer = epd_hl_get_framebuffer(&self->hl);
    if (framebuffer == NULL) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to get framebuffer"));
    
    return mp_obj_new_int((mp_int_t)framebuffer);
}

STATIC mp_obj_t papers3_epdiy_update_screen(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Not initialized"));
    
    int mode = (n_args > 1) ? mp_obj_get_int(args[1]) : MODE_GC16;
    
    epd_poweron();
    epd_hl_update_screen(&self->hl, (enum EpdDrawMode)mode, s_epd_temperature);
    epd_poweroff();
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_update_area(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Not initialized"));
    
    if (n_args < 5) mp_raise_ValueError(MP_ERROR_TEXT("Need x, y, w, h"));
    
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int w = mp_obj_get_int(args[3]);
    int h = mp_obj_get_int(args[4]);
    int mode = (n_args > 5) ? mp_obj_get_int(args[5]) : MODE_GC16;
    
    EpdRect area = { .x = x, .y = y, .width = w, .height = h };
    
    epd_poweron();
    epd_hl_update_area(&self->hl, (enum EpdDrawMode)mode, s_epd_temperature, area);
    epd_poweroff();
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_clear(mp_obj_t self_in) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->initialized) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Not initialized"));
    
    epd_poweron();
    epd_clear();
    epd_poweroff();
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_get_width(mp_obj_t self_in) { return mp_obj_new_int(PAPERS3_WIDTH); }
STATIC mp_obj_t papers3_epdiy_get_height(mp_obj_t self_in) { return mp_obj_new_int(PAPERS3_HEIGHT); }

STATIC mp_obj_t papers3_epdiy_set_temperature(mp_obj_t self_in, mp_obj_t temp_obj) {
    s_epd_temperature = mp_obj_get_float(temp_obj);
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_get_temperature(mp_obj_t self_in) {
    return mp_obj_new_float(s_epd_temperature);
}

// Drawing wrapper helpers
STATIC mp_obj_t papers3_epdiy_draw_pixel(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Not initialized"));
    
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    uint8_t color = mp_obj_get_int(args[3]);
    
    uint8_t* fb = epd_hl_get_framebuffer(&self->hl);
    if (fb) epd_draw_pixel(x, y, color, fb);
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_draw_line(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Not initialized"));
    
    int x0 = mp_obj_get_int(args[1]);
    int y0 = mp_obj_get_int(args[2]);
    int x1 = mp_obj_get_int(args[3]);
    int y1 = mp_obj_get_int(args[4]);
    uint8_t color = mp_obj_get_int(args[5]);
    
    uint8_t* fb = epd_hl_get_framebuffer(&self->hl);
    if (fb) epd_draw_line(x0, y0, x1, y1, color, fb);
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_draw_rect(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Not initialized"));
    
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int w = mp_obj_get_int(args[3]);
    int h = mp_obj_get_int(args[4]);
    uint8_t color = mp_obj_get_int(args[5]);
    
    uint8_t* fb = epd_hl_get_framebuffer(&self->hl);
    if (fb) epd_draw_rect((EpdRect){x,y,w,h}, color, fb);
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_fill_rect(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Not initialized"));
    
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int w = mp_obj_get_int(args[3]);
    int h = mp_obj_get_int(args[4]);
    uint8_t color = mp_obj_get_int(args[5]);
    
    uint8_t* fb = epd_hl_get_framebuffer(&self->hl);
    if (fb) epd_fill_rect((EpdRect){x,y,w,h}, color, fb);
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_draw_circle(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Not initialized"));
    
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int r = mp_obj_get_int(args[3]);
    uint8_t color = mp_obj_get_int(args[4]);
    
    uint8_t* fb = epd_hl_get_framebuffer(&self->hl);
    if (fb) epd_draw_circle(x, y, r, color, fb);
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_fill_circle(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Not initialized"));
    
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int r = mp_obj_get_int(args[3]);
    uint8_t color = mp_obj_get_int(args[4]);
    
    uint8_t* fb = epd_hl_get_framebuffer(&self->hl);
    if (fb) epd_fill_circle(x, y, r, color, fb);
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_draw_triangle(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Not initialized"));
    
    int x0 = mp_obj_get_int(args[1]);
    int y0 = mp_obj_get_int(args[2]);
    int x1 = mp_obj_get_int(args[3]);
    int y1 = mp_obj_get_int(args[4]);
    int x2 = mp_obj_get_int(args[5]);
    int y2 = mp_obj_get_int(args[6]);
    uint8_t color = mp_obj_get_int(args[7]);
    
    uint8_t* fb = epd_hl_get_framebuffer(&self->hl);
    if (fb) epd_draw_triangle(x0, y0, x1, y1, x2, y2, color, fb);
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_fill_triangle(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Not initialized"));
    
    int x0 = mp_obj_get_int(args[1]);
    int y0 = mp_obj_get_int(args[2]);
    int x1 = mp_obj_get_int(args[3]);
    int y1 = mp_obj_get_int(args[4]);
    int x2 = mp_obj_get_int(args[5]);
    int y2 = mp_obj_get_int(args[6]);
    uint8_t color = mp_obj_get_int(args[7]);
    
    uint8_t* fb = epd_hl_get_framebuffer(&self->hl);
    if (fb) epd_fill_triangle(x0, y0, x1, y1, x2, y2, color, fb);
    return mp_const_none;
}

STATIC mp_obj_t papers3_epdiy_draw_text(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Not initialized"));
    
    const char* text = mp_obj_str_get_str(args[1]);
    int x = mp_obj_get_int(args[2]);
    int y = mp_obj_get_int(args[3]);
    uint8_t color = mp_obj_get_int(args[4]);
    
    uint8_t* fb = epd_hl_get_framebuffer(&self->hl);
    if (fb == NULL) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to get framebuffer"));
    
    const EpdFont* font = &Chinese24;
    EpdFontProperties props = epd_font_properties_default();
    props.fg_color = (color >> 4) & 0x0F;
    props.bg_color = 0x0F;
    props.fallback_glyph = '?';
    props.flags = EPD_DRAW_BACKGROUND;
    
    enum EpdDrawError err = epd_write_string(font, (char*)text, &x, &y, fb, &props);
    if (err != EPD_DRAW_SUCCESS) {
        ESP_LOGE(TAG, "Draw text failed: %d", err);
    }
    return mp_const_none;
}

// Shortcut aliases
STATIC mp_obj_t papers3_epdiy_update(mp_obj_t self_in) {
    return papers3_epdiy_update_screen(1, &self_in);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_init_obj, papers3_epdiy_init);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_deinit_obj, papers3_epdiy_deinit);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_get_framebuffer_obj, papers3_epdiy_get_framebuffer);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_update_screen_obj, 1, 2, papers3_epdiy_update_screen);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_update_area_obj, 5, 6, papers3_epdiy_update_area);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_update_obj, papers3_epdiy_update);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_clear_obj, papers3_epdiy_clear);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_get_width_obj, papers3_epdiy_get_width);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_get_height_obj, papers3_epdiy_get_height);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(papers3_epdiy_set_temperature_obj, papers3_epdiy_set_temperature);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_get_temperature_obj, papers3_epdiy_get_temperature);

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_draw_pixel_obj, 4, 4, papers3_epdiy_draw_pixel);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_draw_line_obj, 6, 6, papers3_epdiy_draw_line);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_draw_rect_obj, 6, 6, papers3_epdiy_draw_rect);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_fill_rect_obj, 6, 6, papers3_epdiy_fill_rect);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_draw_circle_obj, 5, 5, papers3_epdiy_draw_circle);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_fill_circle_obj, 5, 5, papers3_epdiy_fill_circle);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_draw_triangle_obj, 8, 8, papers3_epdiy_draw_triangle);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_fill_triangle_obj, 8, 8, papers3_epdiy_fill_triangle);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_draw_text_obj, 5, 5, papers3_epdiy_draw_text);

STATIC const mp_rom_map_elem_t papers3_epdiy_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_epdiy_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_epdiy_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&papers3_epdiy_deinit_obj) }, // Auto-deinit on GC
    { MP_ROM_QSTR(MP_QSTR_get_framebuffer), MP_ROM_PTR(&papers3_epdiy_get_framebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_update_screen), MP_ROM_PTR(&papers3_epdiy_update_screen_obj) },
    { MP_ROM_QSTR(MP_QSTR_update_area), MP_ROM_PTR(&papers3_epdiy_update_area_obj) },
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&papers3_epdiy_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&papers3_epdiy_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&papers3_epdiy_get_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&papers3_epdiy_get_height_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_temperature), MP_ROM_PTR(&papers3_epdiy_set_temperature_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_temperature), MP_ROM_PTR(&papers3_epdiy_get_temperature_obj) },
    
    { MP_ROM_QSTR(MP_QSTR_draw_pixel), MP_ROM_PTR(&papers3_epdiy_draw_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_line), MP_ROM_PTR(&papers3_epdiy_draw_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_rect), MP_ROM_PTR(&papers3_epdiy_draw_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&papers3_epdiy_fill_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_circle), MP_ROM_PTR(&papers3_epdiy_draw_circle_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_circle), MP_ROM_PTR(&papers3_epdiy_fill_circle_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_triangle), MP_ROM_PTR(&papers3_epdiy_draw_triangle_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_triangle), MP_ROM_PTR(&papers3_epdiy_fill_triangle_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_text), MP_ROM_PTR(&papers3_epdiy_draw_text_obj) },
};
STATIC MP_DEFINE_CONST_DICT(papers3_epdiy_locals_dict, papers3_epdiy_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    papers3_epdiy_type,
    MP_QSTR_EPDiy,
    MP_TYPE_FLAG_NONE,
    make_new, papers3_epdiy_make_new,
    locals_dict, &papers3_epdiy_locals_dict
);