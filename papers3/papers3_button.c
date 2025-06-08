#include "py/runtime.h"
#include "py/obj.h"
#include "py/objint.h"

// ESP-IDF includes
#include "driver/gpio.h"
#include "esp_log.h"

static const char* TAG = "papers3_button";

// 按钮GPIO配置 (参考papers3-esp-demo/main/main.cpp)
#define CHG_STATE_GPIO GPIO_NUM_4  // GPIO 4: CHG_STATE (0: charging, 1: full)
#define USB_DET_GPIO   GPIO_NUM_5  // GPIO 5: USB_DET (1: USB connected)

// 按钮对象类型定义
typedef struct _papers3_button_obj_t {
    mp_obj_base_t base;
    bool initialized;
} papers3_button_obj_t;

// 按钮类构造函数
static mp_obj_t papers3_button_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    
    papers3_button_obj_t *self = mp_obj_malloc(papers3_button_obj_t, type);
    self->initialized = false;
    
    return MP_OBJ_FROM_PTR(self);
}

// 初始化按钮
static mp_obj_t papers3_button_init(mp_obj_t self_in) {
    papers3_button_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        return mp_const_true;
    }
    
    // 配置CHG_STATE GPIO为输入模式
    gpio_config_t chg_conf = {
        .pin_bit_mask = (1ULL << CHG_STATE_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,  // 启用上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t ret = gpio_config(&chg_conf);
    if (ret != ESP_OK) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("CHG_STATE GPIO config failed"));
    }
    
    // 配置USB_DET GPIO为输入模式
    gpio_config_t usb_conf = {
        .pin_bit_mask = (1ULL << USB_DET_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,  // 启用上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    ret = gpio_config(&usb_conf);
    if (ret != ESP_OK) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("USB_DET GPIO config failed"));
    }
    
    self->initialized = true;
    
    mp_printf(&mp_plat_print, "Buttons initialized (CHG_STATE: GPIO %d, USB_DET: GPIO %d)\n", 
              CHG_STATE_GPIO, USB_DET_GPIO);
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_button_init_obj, papers3_button_init);

// 读取充电状态
static mp_obj_t papers3_button_charge_state(mp_obj_t self_in) {
    papers3_button_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Buttons not initialized"));
    }
    
    int level = gpio_get_level(CHG_STATE_GPIO);
    // 0: charging, 1: full (根据demo注释)
    return mp_obj_new_bool(level == 1);  // True表示充满，False表示充电中
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_button_charge_state_obj, papers3_button_charge_state);

// 读取USB连接状态
static mp_obj_t papers3_button_usb_connected(mp_obj_t self_in) {
    papers3_button_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Buttons not initialized"));
    }
    
    int level = gpio_get_level(USB_DET_GPIO);
    // 1: USB connected (根据demo注释)
    return mp_obj_new_bool(level == 1);
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_button_usb_connected_obj, papers3_button_usb_connected);

// 获取所有状态信息
static mp_obj_t papers3_button_status(mp_obj_t self_in) {
    papers3_button_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Buttons not initialized"));
    }
    
    int chg_level = gpio_get_level(CHG_STATE_GPIO);
    int usb_level = gpio_get_level(USB_DET_GPIO);
    
    // 创建字典返回状态
    mp_obj_t dict = mp_obj_new_dict(2);
    mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_charge_full), mp_obj_new_bool(chg_level == 1));
    mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_usb_connected), mp_obj_new_bool(usb_level == 1));
    
    return dict;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_button_status_obj, papers3_button_status);

// 反初始化按钮
static mp_obj_t papers3_button_deinit(mp_obj_t self_in) {
    papers3_button_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        self->initialized = false;
        mp_printf(&mp_plat_print, "Buttons deinitialized\n");
    }
    
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_button_deinit_obj, papers3_button_deinit);

// 按钮类方法表
static const mp_rom_map_elem_t papers3_button_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_button_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_charge_state), MP_ROM_PTR(&papers3_button_charge_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_usb_connected), MP_ROM_PTR(&papers3_button_usb_connected_obj) },
    { MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&papers3_button_status_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_button_deinit_obj) },
};
static MP_DEFINE_CONST_DICT(papers3_button_locals_dict, papers3_button_locals_dict_table);

// 按钮类型定义
MP_DEFINE_CONST_OBJ_TYPE(
    papers3_button_type,
    MP_QSTR_Button,
    MP_TYPE_FLAG_NONE,
    make_new, papers3_button_make_new,
    locals_dict, &papers3_button_locals_dict
); 