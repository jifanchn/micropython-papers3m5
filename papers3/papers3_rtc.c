/*
 * Papers3 BM8563 RTC模块
 * 
 * 硬件: BM8563实时时钟 (I2C地址: 0x51)
 * 功能: 时间日期读写，闹钟设置
 * 设计: 面向对象接口 papers3.RTC()
 * 接口: 使用MicroPython的machine.I2C
 */

#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"
#include "py/mperrno.h"

#include "esp_log.h"
#include "py/builtin.h"

#define TAG "papers3_rtc"

// BM8563 I2C配置
#define BM8563_I2C_ADDR         0x51
#define BM8563_I2C_SDA_PIN      8
#define BM8563_I2C_SCL_PIN      9

// BM8563 寄存器地址
#define BM8563_CONTROL_STATUS1  0x00
#define BM8563_CONTROL_STATUS2  0x01
#define BM8563_SECONDS          0x02
#define BM8563_MINUTES          0x03
#define BM8563_HOURS            0x04
#define BM8563_DAYS             0x05
#define BM8563_WEEKDAYS         0x06
#define BM8563_MONTHS           0x07
#define BM8563_YEARS            0x08
#define BM8563_MINUTE_ALARM     0x09
#define BM8563_HOUR_ALARM       0x0A
#define BM8563_DAY_ALARM        0x0B
#define BM8563_WEEKDAY_ALARM    0x0C
#define BM8563_TIMER_CONTROL    0x0E
#define BM8563_TIMER            0x0F

// BM8563 控制位
#define BM8563_ALARM_ENABLE     0x80
#define BM8563_ALARM_DISABLE    0x7F

// 结构体定义
typedef struct _papers3_rtc_obj_t {
    mp_obj_base_t base;
    bool initialized;
    mp_obj_t i2c_obj;  // MicroPython I2C对象
} papers3_rtc_obj_t;

// BCD转换函数
static uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) + (dec % 10);
}

// I2C读写函数 - 使用MicroPython的I2C接口
static int bm8563_i2c_read(papers3_rtc_obj_t *self, uint8_t reg_addr, uint8_t *data, size_t len) {
    if (!self->i2c_obj || self->i2c_obj == mp_const_none) {
        return -1;
    }
    
    mp_obj_t dest[3];
    
    // 准备参数: addr, memaddr, nbytes
    dest[0] = mp_obj_new_int(BM8563_I2C_ADDR);
    dest[1] = mp_obj_new_int(reg_addr);
    dest[2] = mp_obj_new_int(len);
    
    // 调用 i2c.readfrom_mem(addr, memaddr, nbytes)
    mp_obj_t args[5] = {self->i2c_obj, MP_OBJ_NEW_QSTR(MP_QSTR_readfrom_mem), dest[0], dest[1], dest[2]};
    mp_obj_t result = mp_call_method_n_kw(3, 0, args);
    
    if (result != mp_const_none) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(result, &bufinfo, MP_BUFFER_READ);
        if (bufinfo.len >= len) {
            memcpy(data, bufinfo.buf, len);
            return 0;
        }
    }
    return -1;
}

static int bm8563_i2c_write(papers3_rtc_obj_t *self, uint8_t reg_addr, uint8_t data) {
    if (!self->i2c_obj || self->i2c_obj == mp_const_none) {
        return -1;
    }
    
    mp_obj_t dest[3];
    
    // 准备参数: addr, memaddr, data
    dest[0] = mp_obj_new_int(BM8563_I2C_ADDR);
    dest[1] = mp_obj_new_int(reg_addr);
    dest[2] = mp_obj_new_bytes(&data, 1);
    
    // 调用 i2c.writeto_mem(addr, memaddr, data)
    mp_obj_t args[5] = {self->i2c_obj, MP_OBJ_NEW_QSTR(MP_QSTR_writeto_mem), dest[0], dest[1], dest[2]};
    mp_obj_t result = mp_call_method_n_kw(3, 0, args);
    
    return (result != mp_const_none) ? 0 : -1;
}

// RTC构造函数
static mp_obj_t papers3_rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    
    // 前向声明类型
    extern const mp_obj_type_t papers3_rtc_type;
    papers3_rtc_obj_t *self = mp_obj_malloc(papers3_rtc_obj_t, &papers3_rtc_type);
    self->initialized = false;
    self->i2c_obj = mp_const_none;
    
    return MP_OBJ_FROM_PTR(self);
}

// init() 方法
static mp_obj_t papers3_rtc_init(mp_obj_t self_in) {
    papers3_rtc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    ESP_LOGI(TAG, "初始化BM8563 RTC");
    
    // 创建I2C对象 machine.I2C(0, scl=Pin(9), sda=Pin(8), freq=100000)
    mp_obj_t args[5];
    
    // 导入machine模块
    mp_obj_t machine_module = mp_import_name(MP_QSTR_machine, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
    
    // 获取Pin和I2C类
    mp_obj_t pin_class = mp_load_attr(machine_module, MP_QSTR_Pin);
    mp_obj_t i2c_class = mp_load_attr(machine_module, MP_QSTR_I2C);
    
    // 创建Pin对象
    mp_obj_t scl_pin = mp_call_function_1(pin_class, mp_obj_new_int(BM8563_I2C_SCL_PIN));
    mp_obj_t sda_pin = mp_call_function_1(pin_class, mp_obj_new_int(BM8563_I2C_SDA_PIN));
    
    // 创建I2C对象: I2C(0, scl=scl_pin, sda=sda_pin, freq=100000)
    args[0] = mp_obj_new_int(0);  // I2C ID
    
    mp_obj_t kw_args[3];
    kw_args[0] = scl_pin;
    kw_args[1] = sda_pin; 
    kw_args[2] = mp_obj_new_int(100000);  // 100kHz
    
    static const qstr kw_names[3] = {MP_QSTR_scl, MP_QSTR_sda, MP_QSTR_freq};
    
    // 构建完整的参数数组 (总共7个参数)
    mp_obj_t all_args[7] = {args[0], MP_OBJ_NEW_QSTR(kw_names[0]), kw_args[0], 
                            MP_OBJ_NEW_QSTR(kw_names[1]), kw_args[1], 
                            MP_OBJ_NEW_QSTR(kw_names[2]), kw_args[2]};
    self->i2c_obj = mp_call_function_n_kw(i2c_class, 1, 3, all_args);
    
    // 检查RTC是否响应
    uint8_t status1;
    if (bm8563_i2c_read(self, BM8563_CONTROL_STATUS1, &status1, 1) != 0) {
        ESP_LOGE(TAG, "无法读取BM8563状态寄存器");
        mp_raise_OSError(MP_EIO);
    }
    
    // 清除控制/状态寄存器
    if (bm8563_i2c_write(self, BM8563_CONTROL_STATUS1, 0x00) != 0) {
        mp_raise_OSError(MP_EIO);
    }
    
    if (bm8563_i2c_write(self, BM8563_CONTROL_STATUS2, 0x00) != 0) {
        mp_raise_OSError(MP_EIO);
    }
    
    self->initialized = true;
    ESP_LOGI(TAG, "BM8563 RTC初始化成功");
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_rtc_init_obj, papers3_rtc_init);

// deinit() 方法
static mp_obj_t papers3_rtc_deinit(mp_obj_t self_in) {
    papers3_rtc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        self->i2c_obj = mp_const_none;
        self->initialized = false;
        ESP_LOGI(TAG, "BM8563 RTC已反初始化");
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_rtc_deinit_obj, papers3_rtc_deinit);

// datetime() 方法 - 读取或设置时间 (year, month, day, weekday, hour, minute, second)
static mp_obj_t papers3_rtc_datetime(size_t n_args, const mp_obj_t *args) {
    papers3_rtc_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_ValueError(MP_ERROR_TEXT("RTC not initialized"));
    }
    
    if (n_args == 1) {
        // 读取时间
        uint8_t rtc_data[7];
        if (bm8563_i2c_read(self, BM8563_SECONDS, rtc_data, 7) != 0) {
            mp_raise_OSError(MP_EIO);
        }
        
        mp_obj_t tuple[8] = {
            mp_obj_new_int(2000 + bcd_to_dec(rtc_data[6] & 0xFF)),  // year
            mp_obj_new_int(bcd_to_dec(rtc_data[5] & 0x1F)),         // month
            mp_obj_new_int(bcd_to_dec(rtc_data[3] & 0x3F)),         // day
            mp_obj_new_int(rtc_data[4] & 0x07),                     // weekday
            mp_obj_new_int(bcd_to_dec(rtc_data[2] & 0x3F)),         // hour
            mp_obj_new_int(bcd_to_dec(rtc_data[1] & 0x7F)),         // minute
            mp_obj_new_int(bcd_to_dec(rtc_data[0] & 0x7F)),         // second
            mp_obj_new_int(0)                                       // subsecond
        };
        
        return mp_obj_new_tuple(8, tuple);
    } else if (n_args == 2) {
        // 设置时间 - 参数: (year, month, day, weekday, hour, minute, second, subsecond)
        mp_obj_t *datetime_tuple;
        size_t tuple_len;
        mp_obj_get_array(args[1], &tuple_len, &datetime_tuple);
        
        if (tuple_len < 7) {
            mp_raise_ValueError(MP_ERROR_TEXT("datetime tuple should have at least 7 elements"));
        }
        
        int year = mp_obj_get_int(datetime_tuple[0]);
        int month = mp_obj_get_int(datetime_tuple[1]);
        int day = mp_obj_get_int(datetime_tuple[2]);
        int weekday = mp_obj_get_int(datetime_tuple[3]);
        int hour = mp_obj_get_int(datetime_tuple[4]);
        int minute = mp_obj_get_int(datetime_tuple[5]);
        int second = mp_obj_get_int(datetime_tuple[6]);
        
        // 写入RTC
        uint8_t rtc_data[7] = {
            dec_to_bcd(second),
            dec_to_bcd(minute),
            dec_to_bcd(hour),
            dec_to_bcd(day),
            weekday & 0x07,
            dec_to_bcd(month),
            dec_to_bcd(year - 2000)
        };
        
        for (int i = 0; i < 7; i++) {
            if (bm8563_i2c_write(self, BM8563_SECONDS + i, rtc_data[i]) != 0) {
                mp_raise_OSError(MP_EIO);
            }
        }
        
        return mp_const_none;
    } else {
        mp_raise_TypeError(MP_ERROR_TEXT("datetime() takes 0 or 1 argument"));
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_rtc_datetime_obj, 1, 2, papers3_rtc_datetime);

// alarm() 方法 - 设置闹钟 (day, hour, minute)
static mp_obj_t papers3_rtc_alarm(size_t n_args, const mp_obj_t *args) {
    papers3_rtc_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_ValueError(MP_ERROR_TEXT("RTC not initialized"));
    }
    
    if (n_args == 1) {
        // 读取闹钟设置
        uint8_t alarm_data[4];
        if (bm8563_i2c_read(self, BM8563_MINUTE_ALARM, alarm_data, 4) != 0) {
            mp_raise_OSError(MP_EIO);
        }
        
        bool enabled = !(alarm_data[0] & BM8563_ALARM_ENABLE);
        
        mp_obj_t tuple[4] = {
            mp_obj_new_bool(enabled),
            mp_obj_new_int(bcd_to_dec(alarm_data[2] & 0x3F)),  // day
            mp_obj_new_int(bcd_to_dec(alarm_data[1] & 0x3F)),  // hour
            mp_obj_new_int(bcd_to_dec(alarm_data[0] & 0x7F))   // minute
        };
        
        return mp_obj_new_tuple(4, tuple);
    } else if (n_args == 4) {
        // 设置闹钟 (day, hour, minute)
        int day = mp_obj_get_int(args[1]);
        int hour = mp_obj_get_int(args[2]);
        int minute = mp_obj_get_int(args[3]);
        
        // 写入闹钟寄存器
        if (bm8563_i2c_write(self, BM8563_MINUTE_ALARM, dec_to_bcd(minute)) != 0) {
            mp_raise_OSError(MP_EIO);
        }
        
        if (bm8563_i2c_write(self, BM8563_HOUR_ALARM, dec_to_bcd(hour)) != 0) {
            mp_raise_OSError(MP_EIO);
        }
        
        if (bm8563_i2c_write(self, BM8563_DAY_ALARM, dec_to_bcd(day)) != 0) {
            mp_raise_OSError(MP_EIO);
        }
        
        // 禁用weekday闹钟
        if (bm8563_i2c_write(self, BM8563_WEEKDAY_ALARM, BM8563_ALARM_ENABLE) != 0) {
            mp_raise_OSError(MP_EIO);
        }
        
        // 启用闹钟
        uint8_t status2;
        if (bm8563_i2c_read(self, BM8563_CONTROL_STATUS2, &status2, 1) != 0) {
            mp_raise_OSError(MP_EIO);
        }
        
        status2 |= 0x02;  // 启用闹钟中断
        if (bm8563_i2c_write(self, BM8563_CONTROL_STATUS2, status2) != 0) {
            mp_raise_OSError(MP_EIO);
        }
        
        return mp_const_none;
    } else {
        mp_raise_TypeError(MP_ERROR_TEXT("alarm() takes 0 or 3 arguments"));
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_rtc_alarm_obj, 1, 4, papers3_rtc_alarm);

// 方法表
static const mp_rom_map_elem_t papers3_rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_rtc_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_rtc_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&papers3_rtc_datetime_obj) },
    { MP_ROM_QSTR(MP_QSTR_alarm), MP_ROM_PTR(&papers3_rtc_alarm_obj) },
};
static MP_DEFINE_CONST_DICT(papers3_rtc_locals_dict, papers3_rtc_locals_dict_table);

// 类型定义
MP_DEFINE_CONST_OBJ_TYPE(
    papers3_rtc_type,
    MP_QSTR_RTC,
    MP_TYPE_FLAG_NONE,
    make_new, papers3_rtc_make_new,
    locals_dict, &papers3_rtc_locals_dict
); 