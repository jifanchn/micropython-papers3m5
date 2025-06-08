/*
 * Papers3 Buzzer Sub-module
 * Buzzer control for Papers3 board
 */

#include "py/runtime.h"
#include "py/obj.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

// static const char* TAG = "papers3_buzzer";  // Unused variable

// Papers3 buzzer GPIO - 根据用户参考实现
#define BUZZER_GPIO GPIO_NUM_21
#define BUZZER_CHANNEL LEDC_CHANNEL_0
#define BUZZER_TIMER LEDC_TIMER_0

static bool buzzer_initialized = false;

// Initialize buzzer
static mp_obj_t papers3_buzzer_init(void) {
    if (buzzer_initialized) {
        return mp_const_true;
    }
    
    // Configure LEDC timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = BUZZER_TIMER,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);
    
    // Configure LEDC channel
    ledc_channel_config_t channel_conf = {
        .gpio_num = BUZZER_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BUZZER_CHANNEL,
        .timer_sel = BUZZER_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel_conf);
    
    buzzer_initialized = true;
    mp_printf(&mp_plat_print, "Papers3 buzzer initialized (GPIO %d)\n", BUZZER_GPIO);
    
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_0(papers3_buzzer_init_obj, papers3_buzzer_init);

// Play a tone
static mp_obj_t papers3_buzzer_beep(mp_obj_t freq_obj, mp_obj_t duration_obj) {
    if (!buzzer_initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Buzzer not initialized"));
    }
    
    int freq = mp_obj_get_int(freq_obj);
    int duration = mp_obj_get_int(duration_obj);
    
    if (freq < 100 || freq > 10000) {
        mp_raise_ValueError(MP_ERROR_TEXT("Frequency must be 100-10000 Hz"));
    }
    
    if (duration < 10 || duration > 5000) {
        mp_raise_ValueError(MP_ERROR_TEXT("Duration must be 10-5000 ms"));
    }
    
    // Set frequency
    ledc_set_freq(LEDC_LOW_SPEED_MODE, BUZZER_TIMER, freq);
    
    // Start beep (50% duty cycle)
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 4096);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
    
    mp_printf(&mp_plat_print, "Buzzer: %d Hz for %d ms\n", freq, duration);
    
    // Wait for duration
    vTaskDelay(pdMS_TO_TICKS(duration));
    
    // Stop beep
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(papers3_buzzer_beep_obj, papers3_buzzer_beep);

// Stop buzzer
static mp_obj_t papers3_buzzer_stop(void) {
    if (!buzzer_initialized) {
        return mp_const_false;
    }
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
    
    mp_printf(&mp_plat_print, "Buzzer stopped\n");
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_0(papers3_buzzer_stop_obj, papers3_buzzer_stop);

// Deinitialize buzzer
static mp_obj_t papers3_buzzer_deinit(void) {
    if (!buzzer_initialized) {
        return mp_const_false;
    }
    
    ledc_stop(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
    buzzer_initialized = false;
    
    mp_printf(&mp_plat_print, "Papers3 buzzer deinitialized\n");
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_0(papers3_buzzer_deinit_obj, papers3_buzzer_deinit);

// Buzzer sub-module globals table
static const mp_rom_map_elem_t papers3_buzzer_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_papers3_buzzer) },
    
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_buzzer_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_buzzer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_beep), MP_ROM_PTR(&papers3_buzzer_beep_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&papers3_buzzer_stop_obj) },
};
static MP_DEFINE_CONST_DICT(papers3_buzzer_module_globals, papers3_buzzer_module_globals_table);

// Define the Buzzer sub-module (for backwards compatibility through main module)
const mp_obj_module_t papers3_buzzer_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&papers3_buzzer_module_globals,
};

// Remove independent registration - only support OOP way
// MP_REGISTER_MODULE(MP_QSTR_papers3_buzzer, papers3_buzzer_module);

// Object type for Buzzer class instances
typedef struct _papers3_buzzer_obj_t {
    mp_obj_base_t base;
    bool initialized;
} papers3_buzzer_obj_t;

// Buzzer class constructor
static mp_obj_t papers3_buzzer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    
    papers3_buzzer_obj_t *self = mp_obj_malloc(papers3_buzzer_obj_t, type);
    self->initialized = false;
    
    return MP_OBJ_FROM_PTR(self);
}

// Buzzer class methods using the instance
static mp_obj_t papers3_buzzer_obj_init(mp_obj_t self_in) {
    papers3_buzzer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        return mp_const_true;
    }
    
    // Configure LEDC timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = BUZZER_TIMER,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);
    
    // Configure LEDC channel
    ledc_channel_config_t channel_conf = {
        .gpio_num = BUZZER_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BUZZER_CHANNEL,
        .timer_sel = BUZZER_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel_conf);
    
    self->initialized = true;
    mp_printf(&mp_plat_print, "Buzzer initialized (GPIO %d)\n", BUZZER_GPIO);
    
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_buzzer_obj_init_obj, papers3_buzzer_obj_init);

static mp_obj_t papers3_buzzer_obj_beep(mp_obj_t self_in, mp_obj_t freq_obj, mp_obj_t duration_obj) {
    papers3_buzzer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Buzzer not initialized"));
    }
    
    int freq = mp_obj_get_int(freq_obj);
    int duration = mp_obj_get_int(duration_obj);
    
    if (freq < 100 || freq > 10000) {
        mp_raise_ValueError(MP_ERROR_TEXT("Frequency must be 100-10000 Hz"));
    }
    
    if (duration < 10 || duration > 5000) {
        mp_raise_ValueError(MP_ERROR_TEXT("Duration must be 10-5000 ms"));
    }
    
    // Set frequency
    ledc_set_freq(LEDC_LOW_SPEED_MODE, BUZZER_TIMER, freq);
    
    // Start beep (50% duty cycle)
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 4096);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
    
    mp_printf(&mp_plat_print, "Buzzer: %d Hz for %d ms\n", freq, duration);
    
    // Wait for duration
    vTaskDelay(pdMS_TO_TICKS(duration));
    
    // Stop beep
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(papers3_buzzer_obj_beep_obj, papers3_buzzer_obj_beep);

static mp_obj_t papers3_buzzer_obj_deinit(mp_obj_t self_in) {
    papers3_buzzer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        return mp_const_false;
    }
    
    ledc_stop(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
    self->initialized = false;
    
    mp_printf(&mp_plat_print, "Buzzer deinitialized\n");
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_buzzer_obj_deinit_obj, papers3_buzzer_obj_deinit);

// Buzzer class method table
static const mp_rom_map_elem_t papers3_buzzer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_buzzer_obj_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_beep), MP_ROM_PTR(&papers3_buzzer_obj_beep_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_buzzer_obj_deinit_obj) },
};
static MP_DEFINE_CONST_DICT(papers3_buzzer_locals_dict, papers3_buzzer_locals_dict_table);

// Buzzer class type definition
MP_DEFINE_CONST_OBJ_TYPE(
    papers3_buzzer_type,
    MP_QSTR_Buzzer,
    MP_TYPE_FLAG_NONE,
    make_new, papers3_buzzer_make_new,
    locals_dict, &papers3_buzzer_locals_dict
); 