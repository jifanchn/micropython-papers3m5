#include "py/runtime.h"
#include "py/obj.h"
#include "py/objint.h"

// ESP-IDF includes
#include "driver/gpio.h"
#include "esp_log.h"

static const char* TAG = "papers3_led";

// LED GPIO配置 (参考papers3-esp-demo/main/main.cpp)
#define LED_GPIO GPIO_NUM_0  // GPIO 0: LED控制引脚

// LED对象类型定义
typedef struct _papers3_led_obj_t {
    mp_obj_base_t base;
    bool initialized;
    bool state;  // LED当前状态
} papers3_led_obj_t;

// LED类构造函数
static mp_obj_t papers3_led_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    
    papers3_led_obj_t *self = mp_obj_malloc(papers3_led_obj_t, type);
    self->initialized = false;
    self->state = false;
    
    return MP_OBJ_FROM_PTR(self);
}

// 初始化LED
static mp_obj_t papers3_led_init(mp_obj_t self_in) {
    papers3_led_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        return mp_const_true;
    }
    
    // 配置LED GPIO为输出模式
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t ret = gpio_config(&led_conf);
    if (ret != ESP_OK) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("LED GPIO config failed"));
    }
    
    // 初始状态设为关闭
    gpio_set_level(LED_GPIO, 0);
    self->state = false;
    self->initialized = true;
    
    mp_printf(&mp_plat_print, "LED initialized (GPIO %d)\n", LED_GPIO);
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_led_init_obj, papers3_led_init);

// 打开LED
static mp_obj_t papers3_led_on(mp_obj_t self_in) {
    papers3_led_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("LED not initialized"));
    }
    
    gpio_set_level(LED_GPIO, 1);
    self->state = true;
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_led_on_obj, papers3_led_on);

// 关闭LED
static mp_obj_t papers3_led_off(mp_obj_t self_in) {
    papers3_led_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("LED not initialized"));
    }
    
    gpio_set_level(LED_GPIO, 0);
    self->state = false;
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_led_off_obj, papers3_led_off);

// 切换LED状态
static mp_obj_t papers3_led_toggle(mp_obj_t self_in) {
    papers3_led_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("LED not initialized"));
    }
    
    if (self->state) {
        papers3_led_off(self_in);
    } else {
        papers3_led_on(self_in);
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_led_toggle_obj, papers3_led_toggle);

// 设置LED状态
static mp_obj_t papers3_led_set(mp_obj_t self_in, mp_obj_t state_obj) {
    papers3_led_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("LED not initialized"));
    }
    
    bool state = mp_obj_is_true(state_obj);
    
    if (state) {
        papers3_led_on(self_in);
    } else {
        papers3_led_off(self_in);
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(papers3_led_set_obj, papers3_led_set);

// 获取LED状态
static mp_obj_t papers3_led_state(mp_obj_t self_in) {
    papers3_led_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("LED not initialized"));
    }
    
    return mp_obj_new_bool(self->state);
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_led_state_obj, papers3_led_state);

// 反初始化LED
static mp_obj_t papers3_led_deinit(mp_obj_t self_in) {
    papers3_led_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        gpio_set_level(LED_GPIO, 0);  // 关闭LED
        self->state = false;
        self->initialized = false;
        mp_printf(&mp_plat_print, "LED deinitialized\n");
    }
    
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_led_deinit_obj, papers3_led_deinit);

// LED类方法表
static const mp_rom_map_elem_t papers3_led_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_led_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&papers3_led_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&papers3_led_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_toggle), MP_ROM_PTR(&papers3_led_toggle_obj) },
    { MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&papers3_led_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_state), MP_ROM_PTR(&papers3_led_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_led_deinit_obj) },
};
static MP_DEFINE_CONST_DICT(papers3_led_locals_dict, papers3_led_locals_dict_table);

// LED类型定义
MP_DEFINE_CONST_OBJ_TYPE(
    papers3_led_type,
    MP_QSTR_LED,
    MP_TYPE_FLAG_NONE,
    make_new, papers3_led_make_new,
    locals_dict, &papers3_led_locals_dict
); 