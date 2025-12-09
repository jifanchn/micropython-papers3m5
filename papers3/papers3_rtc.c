/*
 * Papers3 BM8563 RTC Module  
 * Refactored to use shared I2C manager
 */

#include "py/runtime.h"
#include "py/obj.h"
#include "py/builtin.h"
#include "py/mperrno.h"

#include "papers3_pins.h"
#include "papers3_i2c.h"
#include "esp_log.h"

#define TAG "papers3_rtc"

#define BM8563_ADDR             0x51
#define BM8563_CONTROL_STATUS1  0x00
#define BM8563_CONTROL_STATUS2  0x01
#define BM8563_SECONDS          0x02
#define BM8563_MINUTE_ALARM     0x09
#define BM8563_ALARM_ENABLE     0x80

typedef struct _papers3_rtc_obj_t {
    mp_obj_base_t base;
    bool initialized;
} papers3_rtc_obj_t;

static uint8_t bcd_to_dec(uint8_t bcd) { return ((bcd >> 4) * 10) + (bcd & 0x0F); }
static uint8_t dec_to_bcd(uint8_t dec) { return ((dec / 10) << 4) + (dec % 10); }

static mp_obj_t papers3_rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    extern const mp_obj_type_t papers3_rtc_type;
    papers3_rtc_obj_t *self = mp_obj_malloc(papers3_rtc_obj_t, &papers3_rtc_type);
    self->initialized = false;
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t papers3_rtc_init(mp_obj_t self_in) {
    papers3_rtc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (papers3_i2c_init() != ESP_OK) {
        mp_raise_OSError(MP_EIO);
    }
    
    // Check RTC
    uint8_t status;
    if (papers3_i2c_read(BM8563_ADDR, BM8563_CONTROL_STATUS1, &status, 1) != ESP_OK) {
        papers3_i2c_deinit();
        mp_raise_OSError(MP_EIO);
    }
    
    // Clear status
    papers3_i2c_write(BM8563_ADDR, BM8563_CONTROL_STATUS1, 0x00);
    papers3_i2c_write(BM8563_ADDR, BM8563_CONTROL_STATUS2, 0x00);
    
    self->initialized = true;
    ESP_LOGI(TAG, "BM8563 RTC initialized");
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_rtc_init_obj, papers3_rtc_init);

static mp_obj_t papers3_rtc_deinit(mp_obj_t self_in) {
    papers3_rtc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->initialized) {
        papers3_i2c_deinit();
        self->initialized = false;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_rtc_deinit_obj, papers3_rtc_deinit);

static mp_obj_t papers3_rtc_datetime(size_t n_args, const mp_obj_t *args) {
    papers3_rtc_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_ValueError(MP_ERROR_TEXT("Not initialized"));
    
    if (n_args == 1) {
        // Read
        uint8_t data[7];
        if (papers3_i2c_read(BM8563_ADDR, BM8563_SECONDS, data, 7) != ESP_OK) {
            mp_raise_OSError(MP_EIO);
        }
        
        uint8_t sec = bcd_to_dec(data[0] & 0x7F);
        uint8_t min = bcd_to_dec(data[1] & 0x7F);
        uint8_t hour = bcd_to_dec(data[2] & 0x3F);
        uint8_t day = bcd_to_dec(data[3] & 0x3F);
        uint8_t wday = bcd_to_dec(data[4] & 0x07);
        uint8_t mon = bcd_to_dec(data[5] & 0x1F);
        uint16_t year = bcd_to_dec(data[6]) + 2000;
        
        mp_obj_t tuple[7] = {
            mp_obj_new_int(year), mp_obj_new_int(mon), mp_obj_new_int(day),
            mp_obj_new_int(wday), mp_obj_new_int(hour), mp_obj_new_int(min), mp_obj_new_int(sec)
        };
        return mp_obj_new_tuple(7, tuple);
    } else {
        // Write
        if (n_args != 8) mp_raise_TypeError(MP_ERROR_TEXT("Requires 7 args"));
        
        uint16_t year = mp_obj_get_int(args[1]);
        uint8_t mon = mp_obj_get_int(args[2]);
        uint8_t day = mp_obj_get_int(args[3]);
        uint8_t wday = mp_obj_get_int(args[4]);
        uint8_t hour = mp_obj_get_int(args[5]);
        uint8_t min = mp_obj_get_int(args[6]);
        uint8_t sec = mp_obj_get_int(args[7]);
        
        uint8_t data[7] = {
            dec_to_bcd(sec), dec_to_bcd(min), dec_to_bcd(hour),
            dec_to_bcd(day), dec_to_bcd(wday), dec_to_bcd(mon),
            dec_to_bcd(year - 2000)
        };
        
        // Write sequentially
        for (int i = 0; i < 7; i++) {
            papers3_i2c_write(BM8563_ADDR, BM8563_SECONDS + i, data[i]);
        }
        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_rtc_datetime_obj, 1, 8, papers3_rtc_datetime);

static mp_obj_t papers3_rtc_alarm(size_t n_args, const mp_obj_t *args) {
    papers3_rtc_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_ValueError(MP_ERROR_TEXT("Not initialized"));
    
    if (n_args == 1) {
        // Read alarm
        uint8_t data[4];
        papers3_i2c_read(BM8563_ADDR, BM8563_MINUTE_ALARM, data, 4);
        bool enabled = !(data[0] & BM8563_ALARM_ENABLE);
        
        if (enabled) {
             uint8_t min = bcd_to_dec(data[0] & 0x7F);
             uint8_t hour = bcd_to_dec(data[1] & 0x3F);
             mp_obj_t tuple[2] = { mp_obj_new_int(hour), mp_obj_new_int(min) };
             return mp_obj_new_tuple(2, tuple);
        }
        return mp_const_none;
    } else {
        // Set alarm
        if (args[1] == mp_const_none) {
            papers3_i2c_write(BM8563_ADDR, BM8563_MINUTE_ALARM, BM8563_ALARM_ENABLE);
        } else {
             uint8_t hour = mp_obj_get_int(args[1]);
             uint8_t min = mp_obj_get_int(args[2]);
             papers3_i2c_write(BM8563_ADDR, BM8563_MINUTE_ALARM, dec_to_bcd(min));
             papers3_i2c_write(BM8563_ADDR, BM8563_MINUTE_ALARM + 1, dec_to_bcd(hour));
             papers3_i2c_write(BM8563_ADDR, BM8563_MINUTE_ALARM + 2, BM8563_ALARM_ENABLE); // Disable day
             papers3_i2c_write(BM8563_ADDR, BM8563_MINUTE_ALARM + 3, BM8563_ALARM_ENABLE); // Disable weekday
        }
        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_rtc_alarm_obj, 1, 3, papers3_rtc_alarm);

static const mp_rom_map_elem_t papers3_rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_rtc_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_rtc_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&papers3_rtc_datetime_obj) },
    { MP_ROM_QSTR(MP_QSTR_alarm), MP_ROM_PTR(&papers3_rtc_alarm_obj) },
};
static MP_DEFINE_CONST_DICT(papers3_rtc_locals_dict, papers3_rtc_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    papers3_rtc_type,
    MP_QSTR_RTC,
    MP_TYPE_FLAG_NONE,
    make_new, papers3_rtc_make_new,
    locals_dict, &papers3_rtc_locals_dict
);