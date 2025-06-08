/*
 * Papers3 EPDiy Module Implementation
 * 基于demo工程ED047TC1Driver.h的正确高级API实现
 * 使用epdiy高级接口，避免直接LCD操作
 */

#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "py/objstr.h"
#include "py/objint.h"

// ESP-IDF 基础头文件
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"

// FreeRTOS 基础头文件
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// GPIO 驱动
#include "driver/gpio.h"

// EPDiy 高级API (参考ED047TC1Driver.h)
#include "epdiy.h"
#include "epd_highlevel.h"
#include "epd_board.h"
#include "lcd_driver.h"  // 关键！包含LCD类型定义
#include <inttypes.h>



// MicroPython 静态定义
#ifndef STATIC
#define STATIC static
#endif

// 外部声明 - Papers3板级定义
// extern const EpdBoardDefinition epd_board_gtxyj;  // 暂时注释，使用默认board

#define TAG "papers3_epdiy"

// Papers3显示参数 (与demo工程一致)
#define PAPERS3_WIDTH  960
#define PAPERS3_HEIGHT 540

// ===== Papers3 板级引脚定义 =====
// 参考 papers3-esp-demo/components/epdiy/src/board/epd_board_gtxyj.c

// 电源和控制引脚 (与gtxyj.c一致)
#define EPD_SPV GPIO_NUM_17  // EPD_SPV
#define EPD_EN  GPIO_NUM_45  // EPD_EN (电源使能)
#define BST_EN  GPIO_NUM_46  // BST_EN (升压使能)
#define EPD_XLE GPIO_NUM_15  // EPD_XLE (锁存使能)

// 控制信号
#define CKV GPIO_NUM_18      // 垂直时钟
#define STH GPIO_NUM_13      // 水平启动

// 时钟引脚
#define CKH GPIO_NUM_16      // 水平时钟

// 数据总线 D0-D7 (8位并行，与gtxyj.c一致)
#define D7 GPIO_NUM_10
#define D6 GPIO_NUM_8
#define D5 GPIO_NUM_11
#define D4 GPIO_NUM_9
#define D3 GPIO_NUM_12
#define D2 GPIO_NUM_7
#define D1 GPIO_NUM_14
#define D0 GPIO_NUM_6

// LCD总线配置 (关键！参考gtxyj.c和v7.c)
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
        // 只有8位数据总线，其他保持未初始化
    }
};

// ===== Papers3 板级函数实现 =====

// 快速GPIO操作（参考demo工程）
inline static void fast_gpio_set_hi(gpio_num_t gpio_num) {
    gpio_set_level(gpio_num, 1);
}

inline static void fast_gpio_set_lo(gpio_num_t gpio_num) {
    gpio_set_level(gpio_num, 0);
}

// 精确延时（参考demo工程）
void IRAM_ATTR busy_delay(uint32_t cycles) {
    volatile uint64_t counts = esp_timer_get_time() + cycles;
    while (esp_timer_get_time() < counts) ;
}

// 初始化函数（参考gtxyj.c的epd_board_init）
static void papers3_board_init(uint32_t epd_row_width) {
    ESP_LOGI(TAG, "Papers3 board init, row width: %"PRIu32, epd_row_width);
    
    // 释放时钟引脚（参考gtxyj.c和v7.c）
    gpio_hold_dis(CKH);
    
    // 配置GPIO方向（参考gtxyj.c）
    gpio_set_direction(EPD_SPV, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_EN, GPIO_MODE_OUTPUT);
    gpio_set_direction(BST_EN, GPIO_MODE_OUTPUT);  
    gpio_set_direction(EPD_XLE, GPIO_MODE_OUTPUT);

    // 初始状态设置（参考gtxyj.c）
    fast_gpio_set_lo(EPD_XLE);   // EPD_XLE
    fast_gpio_set_hi(EPD_SPV);   // EPD_SPV
    fast_gpio_set_lo(EPD_EN);    // EPD_EN
    fast_gpio_set_lo(BST_EN);    // BST_EN
    
    // 关键！LCD初始化（参考gtxyj.c和v7.c）
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

// 清理函数（参考gtxyj.c和v7.c的epd_board_deinit）
static void papers3_board_deinit(void) {
    ESP_LOGI(TAG, "Papers3 board deinit");
    
    // 关键！LCD清理（参考v7.c）
    epd_lcd_deinit();
    
    // GPIO状态复位（参考gtxyj.c）
    fast_gpio_set_lo(EPD_XLE);  // EPD_XLE
    fast_gpio_set_lo(EPD_SPV);  // EPD_SPV
    fast_gpio_set_lo(EPD_EN);   // EPD_EN
    fast_gpio_set_lo(BST_EN);   // BST_EN
}

// 设置控制状态（参考gtxyj.c的epd_board_set_ctrl）
static void papers3_board_set_ctrl(epd_ctrl_state_t* state, const epd_ctrl_state_t* const target) {
    if (!state || !target) return;
    
    if (target->ep_sth) {
        fast_gpio_set_hi(STH);
    } else {
        fast_gpio_set_lo(STH);
    }

    if (target->ep_stv) {
        fast_gpio_set_hi(EPD_SPV);
    } else {
        fast_gpio_set_lo(EPD_SPV);
    }

    if (target->ep_latch_enable) {
        fast_gpio_set_hi(EPD_XLE);
        fast_gpio_set_hi(EPD_XLE);  // 双重设置，确保稳定
    } else {
        fast_gpio_set_lo(EPD_XLE);
        fast_gpio_set_lo(EPD_XLE);
    }
    
    // 更新状态
    *state = *target;
}

// 电源开启（参考gtxyj.c的epd_board_poweron）
static void papers3_board_poweron(epd_ctrl_state_t* state) {
    ESP_LOGI(TAG, "Papers3 board power on");
    
    fast_gpio_set_hi(EPD_EN);   // EPD_EN
    busy_delay(100 * 240);      // 等待稳定
    fast_gpio_set_hi(BST_EN);   // BST_EN
    busy_delay(100 * 240);      // 等待稳定
    fast_gpio_set_hi(EPD_SPV);  // EPD_SPV
    fast_gpio_set_hi(STH);      // STH
}

// VCOM测量（参考demo工程，暂时空实现）
static void papers3_board_measure_vcom(epd_ctrl_state_t* state) {
    ESP_LOGI(TAG, "Papers3 board measure VCOM");
    // Papers3可能不支持VCOM测量，保持空实现
}

// 电源关闭（参考gtxyj.c的epd_board_poweroff）
static void papers3_board_poweroff(epd_ctrl_state_t* state) {
    ESP_LOGI(TAG, "Papers3 board power off");
    
    // 参考gtxyj.c的注释，某些引脚可能不需要关闭
    // fast_gpio_set_lo(BST_EN); // BST_EN
    // busy_delay(10 * 240);
    // fast_gpio_set_lo(EPD_EN);  // EPD_EN
    // busy_delay(100 * 240);
    fast_gpio_set_lo(EPD_SPV);  // EPD_SPV - 这是gtxyj.c实际做的
}

// 设置VCOM电压（参考demo工程set_vcom）
static void papers3_board_set_vcom(int vcom_mv) {
    ESP_LOGI(TAG, "Papers3 board set VCOM: %d mV", vcom_mv);
    // Papers3可能不支持可调VCOM，保持空实现
}

// 获取温度（参考demo工程epd_board_ambient_temperature）
static float papers3_board_get_temperature(void) {
    // 返回固定值，与demo工程一致
    return 25.0f;
}

// GPIO方向设置（可选）
static esp_err_t papers3_board_gpio_set_direction(int pin, bool make_input) {
    // TODO: 实现GPIO方向设置
    return ESP_OK;
}

// GPIO读取（可选）
static bool papers3_board_gpio_read(int pin) {
    // TODO: 实现GPIO读取
    return false;
}

// GPIO写入（可选）
static esp_err_t papers3_board_gpio_write(int pin, bool value) {
    // TODO: 实现GPIO写入
    return ESP_OK;
}

// Papers3 板级定义
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

// ===== Papers3 EPDiy对象类型定义 =====

typedef struct _papers3_epdiy_obj_t {
    mp_obj_base_t base;
    EpdiyHighlevelState hl;  // 高级状态管理 (参考ED047TC1Driver)
    bool initialized;
    int temperature;
} papers3_epdiy_obj_t;

// 前置声明
extern const mp_obj_type_t papers3_epdiy_type;

// ===== 核心功能实现 =====

// 构造函数
STATIC mp_obj_t papers3_epdiy_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    
    papers3_epdiy_obj_t *self = m_new_obj(papers3_epdiy_obj_t);
    self->base.type = &papers3_epdiy_type;
    self->initialized = false;
    self->temperature = 25;  // 默认温度
    
    return MP_OBJ_FROM_PTR(self);
}

// 初始化EPD (参考ED047TC1Driver::init)
STATIC mp_obj_t papers3_epdiy_init(mp_obj_t self_in) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy already initialized"));
    }
    
    // 1. 初始化EPD硬件 (参考demo工程)
    epd_init(&papers3_board, &ED047TC2, EPD_LUT_64K);
    
    // 2. 初始化高级状态 (参考demo工程)
    self->hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
    
    self->initialized = true;
    ESP_LOGI(TAG, "Papers3 EPDiy initialized successfully");
    
    return mp_const_none;
}

// 反初始化
STATIC mp_obj_t papers3_epdiy_deinit(mp_obj_t self_in) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        epd_deinit();
        self->initialized = false;
        ESP_LOGI(TAG, "Papers3 EPDiy deinitialized");
    }
    
    return mp_const_none;
}

// 获取framebuffer (参考ED047TC1Driver::getFrameBuffer)
STATIC mp_obj_t papers3_epdiy_get_framebuffer(mp_obj_t self_in) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy not initialized"));
    }
    
    uint8_t* framebuffer = epd_hl_get_framebuffer(&self->hl);
    if (framebuffer == NULL) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to get framebuffer"));
    }
    
    // 返回framebuffer的内存地址作为整数
    return mp_obj_new_int((mp_int_t)framebuffer);
}

// 更新全屏 (参考ED047TC1Driver::updateDisplay)
STATIC mp_obj_t papers3_epdiy_update_screen(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy not initialized"));
    }
    
    // 默认模式：MODE_GC16 (16级灰度)
    int mode = (n_args > 1) ? mp_obj_get_int(args[1]) : MODE_GC16;
    
    // 电源管理 + 更新 (参考demo工程)
    epd_poweron();
    epd_hl_update_screen(&self->hl, (enum EpdDrawMode)mode, self->temperature);
    epd_poweroff();
    
    return mp_const_none;
}

// 更新区域 (参考ED047TC1Driver::updateDisplay区域版本)
STATIC mp_obj_t papers3_epdiy_update_area(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy not initialized"));
    }
    
    if (n_args < 5) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Need x, y, width, height"));
    }
    
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int w = mp_obj_get_int(args[3]);
    int h = mp_obj_get_int(args[4]);
    int mode = (n_args > 5) ? mp_obj_get_int(args[5]) : MODE_GC16;
    
    EpdRect area = {
        .x = x,
        .y = y,
        .width = w,
        .height = h
    };
    
    // 电源管理 + 区域更新
    epd_poweron();
    epd_hl_update_area(&self->hl, (enum EpdDrawMode)mode, self->temperature, area);
    epd_poweroff();
    
    return mp_const_none;
}

// 清屏 (参考ED047TC1Driver::clear)
STATIC mp_obj_t papers3_epdiy_clear(mp_obj_t self_in) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy not initialized"));
    }
    
    epd_poweron();
    epd_clear();
    epd_poweroff();
    
    return mp_const_none;
}

// 获取显示尺寸
STATIC mp_obj_t papers3_epdiy_get_width(mp_obj_t self_in) {
    return mp_obj_new_int(PAPERS3_WIDTH);
}

STATIC mp_obj_t papers3_epdiy_get_height(mp_obj_t self_in) {
    return mp_obj_new_int(PAPERS3_HEIGHT);
}

// 设置温度
STATIC mp_obj_t papers3_epdiy_set_temperature(mp_obj_t self_in, mp_obj_t temp_obj) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->temperature = mp_obj_get_int(temp_obj);
    return mp_const_none;
}

// 获取温度
STATIC mp_obj_t papers3_epdiy_get_temperature(mp_obj_t self_in) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->temperature);
}

// ===== 绘图功能 =====

// 绘制像素点
STATIC mp_obj_t papers3_epdiy_draw_pixel(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy not initialized"));
    }
    
    if (n_args < 4) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Need x, y, color"));
    }
    
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    uint8_t color = mp_obj_get_int(args[3]);
    
    uint8_t* framebuffer = epd_hl_get_framebuffer(&self->hl);
    if (framebuffer == NULL) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to get framebuffer"));
    }
    
    epd_draw_pixel(x, y, color, framebuffer);
    
    return mp_const_none;
}

// 绘制线条
STATIC mp_obj_t papers3_epdiy_draw_line(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy not initialized"));
    }
    
    if (n_args < 6) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Need x0, y0, x1, y1, color"));
    }
    
    int x0 = mp_obj_get_int(args[1]);
    int y0 = mp_obj_get_int(args[2]);
    int x1 = mp_obj_get_int(args[3]);
    int y1 = mp_obj_get_int(args[4]);
    uint8_t color = mp_obj_get_int(args[5]);
    
    uint8_t* framebuffer = epd_hl_get_framebuffer(&self->hl);
    if (framebuffer == NULL) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to get framebuffer"));
    }
    
    epd_draw_line(x0, y0, x1, y1, color, framebuffer);
    
    return mp_const_none;
}

// 绘制矩形框
STATIC mp_obj_t papers3_epdiy_draw_rect(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy not initialized"));
    }
    
    if (n_args < 6) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Need x, y, width, height, color"));
    }
    
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int width = mp_obj_get_int(args[3]);
    int height = mp_obj_get_int(args[4]);
    uint8_t color = mp_obj_get_int(args[5]);
    
    uint8_t* framebuffer = epd_hl_get_framebuffer(&self->hl);
    if (framebuffer == NULL) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to get framebuffer"));
    }
    
    EpdRect rect = { .x = x, .y = y, .width = width, .height = height };
    epd_draw_rect(rect, color, framebuffer);
    
    return mp_const_none;
}

// 绘制填充矩形
STATIC mp_obj_t papers3_epdiy_fill_rect(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy not initialized"));
    }
    
    if (n_args < 6) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Need x, y, width, height, color"));
    }
    
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int width = mp_obj_get_int(args[3]);
    int height = mp_obj_get_int(args[4]);
    uint8_t color = mp_obj_get_int(args[5]);
    
    uint8_t* framebuffer = epd_hl_get_framebuffer(&self->hl);
    if (framebuffer == NULL) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to get framebuffer"));
    }
    
    EpdRect rect = { .x = x, .y = y, .width = width, .height = height };
    epd_fill_rect(rect, color, framebuffer);
    
    return mp_const_none;
}

// 绘制圆形轮廓
STATIC mp_obj_t papers3_epdiy_draw_circle(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy not initialized"));
    }
    
    if (n_args < 5) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Need x, y, radius, color"));
    }
    
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int radius = mp_obj_get_int(args[3]);
    uint8_t color = mp_obj_get_int(args[4]);
    
    uint8_t* framebuffer = epd_hl_get_framebuffer(&self->hl);
    if (framebuffer == NULL) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to get framebuffer"));
    }
    
    epd_draw_circle(x, y, radius, color, framebuffer);
    
    return mp_const_none;
}

// 绘制填充圆形
STATIC mp_obj_t papers3_epdiy_fill_circle(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy not initialized"));
    }
    
    if (n_args < 5) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Need x, y, radius, color"));
    }
    
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int radius = mp_obj_get_int(args[3]);
    uint8_t color = mp_obj_get_int(args[4]);
    
    uint8_t* framebuffer = epd_hl_get_framebuffer(&self->hl);
    if (framebuffer == NULL) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to get framebuffer"));
    }
    
    epd_fill_circle(x, y, radius, color, framebuffer);
    
    return mp_const_none;
}

// 绘制三角形轮廓
STATIC mp_obj_t papers3_epdiy_draw_triangle(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy not initialized"));
    }
    
    if (n_args < 8) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Need x0, y0, x1, y1, x2, y2, color"));
    }
    
    int x0 = mp_obj_get_int(args[1]);
    int y0 = mp_obj_get_int(args[2]);
    int x1 = mp_obj_get_int(args[3]);
    int y1 = mp_obj_get_int(args[4]);
    int x2 = mp_obj_get_int(args[5]);
    int y2 = mp_obj_get_int(args[6]);
    uint8_t color = mp_obj_get_int(args[7]);
    
    uint8_t* framebuffer = epd_hl_get_framebuffer(&self->hl);
    if (framebuffer == NULL) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to get framebuffer"));
    }
    
    epd_draw_triangle(x0, y0, x1, y1, x2, y2, color, framebuffer);
    
    return mp_const_none;
}

// 绘制填充三角形
STATIC mp_obj_t papers3_epdiy_fill_triangle(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy not initialized"));
    }
    
    if (n_args < 8) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Need x0, y0, x1, y1, x2, y2, color"));
    }
    
    int x0 = mp_obj_get_int(args[1]);
    int y0 = mp_obj_get_int(args[2]);
    int x1 = mp_obj_get_int(args[3]);
    int y1 = mp_obj_get_int(args[4]);
    int x2 = mp_obj_get_int(args[5]);
    int y2 = mp_obj_get_int(args[6]);
    uint8_t color = mp_obj_get_int(args[7]);
    
    uint8_t* framebuffer = epd_hl_get_framebuffer(&self->hl);
    if (framebuffer == NULL) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to get framebuffer"));
    }
    
    epd_fill_triangle(x0, y0, x1, y1, x2, y2, color, framebuffer);
    
    return mp_const_none;
}

// 绘制文字
STATIC mp_obj_t papers3_epdiy_draw_text(size_t n_args, const mp_obj_t *args) {
    papers3_epdiy_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPDiy not initialized"));
    }
    
    if (n_args < 5) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Need x, y, text, color"));
    }
    
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    const char* text = mp_obj_str_get_str(args[3]);
    uint8_t color = mp_obj_get_int(args[4]);
    
    uint8_t* framebuffer = epd_hl_get_framebuffer(&self->hl);
    if (framebuffer == NULL) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to get framebuffer"));
    }
    
    // 设置字体属性
    EpdFontProperties props = epd_font_properties_default();
    props.fg_color = color & 0x0F;  // 前景色 (4位)
    props.bg_color = 0x0F;          // 背景色设为白色
    props.fallback_glyph = 0;       // 缺失字符的后备字符
    props.flags = EPD_DRAW_BACKGROUND; // 绘制背景
    
    // TODO: 实现文字绘制功能
    // 暂时返回错误信息，字体集成问题需要进一步解决
    mp_raise_msg(&mp_type_NotImplementedError, MP_ERROR_TEXT("Text drawing not yet implemented - font integration needed"));
    
    return mp_const_none;
}

// ===== MicroPython 方法表和对象定义 =====

STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_init_obj, papers3_epdiy_init);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_deinit_obj, papers3_epdiy_deinit);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_get_framebuffer_obj, papers3_epdiy_get_framebuffer);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_update_screen_obj, 1, 2, papers3_epdiy_update_screen);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_update_area_obj, 5, 6, papers3_epdiy_update_area);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_clear_obj, papers3_epdiy_clear);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_get_width_obj, papers3_epdiy_get_width);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_get_height_obj, papers3_epdiy_get_height);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(papers3_epdiy_set_temperature_obj, papers3_epdiy_set_temperature);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(papers3_epdiy_get_temperature_obj, papers3_epdiy_get_temperature);

// 绘图函数定义
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_draw_pixel_obj, 4, 4, papers3_epdiy_draw_pixel);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_draw_line_obj, 6, 6, papers3_epdiy_draw_line);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_draw_rect_obj, 6, 6, papers3_epdiy_draw_rect);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_fill_rect_obj, 6, 6, papers3_epdiy_fill_rect);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_draw_circle_obj, 5, 5, papers3_epdiy_draw_circle);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_fill_circle_obj, 5, 5, papers3_epdiy_fill_circle);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_draw_triangle_obj, 8, 8, papers3_epdiy_draw_triangle);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_fill_triangle_obj, 8, 8, papers3_epdiy_fill_triangle);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_draw_text_obj, 5, 5, papers3_epdiy_draw_text);

// 方法字典
STATIC const mp_rom_map_elem_t papers3_epdiy_locals_dict_table[] = {
    // 核心方法
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_epdiy_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_epdiy_deinit_obj) },
    
    // 显示更新
    { MP_ROM_QSTR(MP_QSTR_get_framebuffer), MP_ROM_PTR(&papers3_epdiy_get_framebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_update_screen), MP_ROM_PTR(&papers3_epdiy_update_screen_obj) },
    { MP_ROM_QSTR(MP_QSTR_update_area), MP_ROM_PTR(&papers3_epdiy_update_area_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&papers3_epdiy_clear_obj) },
    
    // 属性访问
    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&papers3_epdiy_get_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&papers3_epdiy_get_height_obj) },
    
    // 温度控制
    { MP_ROM_QSTR(MP_QSTR_set_temperature), MP_ROM_PTR(&papers3_epdiy_set_temperature_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_temperature), MP_ROM_PTR(&papers3_epdiy_get_temperature_obj) },
    
    // 绘图函数
    { MP_ROM_QSTR(MP_QSTR_draw_pixel), MP_ROM_PTR(&papers3_epdiy_draw_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_line), MP_ROM_PTR(&papers3_epdiy_draw_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_rect), MP_ROM_PTR(&papers3_epdiy_draw_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&papers3_epdiy_fill_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_circle), MP_ROM_PTR(&papers3_epdiy_draw_circle_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_circle), MP_ROM_PTR(&papers3_epdiy_fill_circle_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_triangle), MP_ROM_PTR(&papers3_epdiy_draw_triangle_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_triangle), MP_ROM_PTR(&papers3_epdiy_fill_triangle_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_text), MP_ROM_PTR(&papers3_epdiy_draw_text_obj) },
    
    // 常量 - 绘制模式
    { MP_ROM_QSTR(MP_QSTR_MODE_INIT), MP_ROM_INT(MODE_INIT) },
    { MP_ROM_QSTR(MP_QSTR_MODE_DU), MP_ROM_INT(MODE_DU) },
    { MP_ROM_QSTR(MP_QSTR_MODE_DU4), MP_ROM_INT(MODE_DU4) },
    { MP_ROM_QSTR(MP_QSTR_MODE_GC16), MP_ROM_INT(MODE_GC16) },
    { MP_ROM_QSTR(MP_QSTR_MODE_GL16), MP_ROM_INT(MODE_GL16) },
    { MP_ROM_QSTR(MP_QSTR_MODE_A2), MP_ROM_INT(MODE_A2) },
};
STATIC MP_DEFINE_CONST_DICT(papers3_epdiy_locals_dict, papers3_epdiy_locals_dict_table);

// Papers3 EPDiy类型定义
MP_DEFINE_CONST_OBJ_TYPE(
    papers3_epdiy_type,
    MP_QSTR_EPDiy,
    MP_TYPE_FLAG_NONE,
    make_new, papers3_epdiy_make_new,
    locals_dict, &papers3_epdiy_locals_dict
); 