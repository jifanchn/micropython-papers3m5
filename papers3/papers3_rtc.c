/*
 * Papers3 BM8563 RTC Module  
 * 使用ESP-IDF I2C驱动，与陀螺仪共享I2C总线
 */

#include "py/runtime.h"
#include "py/obj.h"
#include "py/builtin.h"
#include "py/mperrno.h"

// ESP-IDF I2C驱动
#include "driver/i2c.h"
#include "esp_log.h"

#define TAG "papers3_rtc"

// BM8563 硬件配置
#define BM8563_I2C_ADDR         0x51
#define BM8563_I2C_PORT         I2C_NUM_0

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

// 外部I2C初始化标志
extern bool g_i2c_initialized;

// 结构体定义
typedef struct _papers3_rtc_obj_t {
    mp_obj_base_t base;
    bool initialized;
} papers3_rtc_obj_t;

// ESP-IDF I2C初始化函数（与gyro模块共享）
static esp_err_t papers3_i2c_init(void) {
    if (g_i2c_initialized) {
        return ESP_OK;  // 已经初始化，直接返回
    }
    
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 41,  // SDA pin
        .scl_io_num = 42,  // SCL pin
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,  // 100kHz
    };
    
    esp_err_t ret = i2c_param_config(BM8563_I2C_PORT, &conf);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = i2c_driver_install(BM8563_I2C_PORT, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        return ret;
    }
    
    g_i2c_initialized = true;
    // ESP_LOGI(TAG, "I2C总线初始化成功");  // 移除可能阻塞启动的日志
    return ESP_OK;
}

// BCD转换函数
static uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) + (dec % 10);
}

// I2C读写函数
static esp_err_t bm8563_i2c_read(uint8_t reg_addr, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BM8563_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BM8563_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(BM8563_I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t bm8563_i2c_write(uint8_t reg_addr, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BM8563_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(BM8563_I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

// RTC构造函数
static mp_obj_t papers3_rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    
    extern const mp_obj_type_t papers3_rtc_type;
    papers3_rtc_obj_t *self = mp_obj_malloc(papers3_rtc_obj_t, &papers3_rtc_type);
    self->initialized = false;
    
    return MP_OBJ_FROM_PTR(self);
}

// init() 方法
static mp_obj_t papers3_rtc_init(mp_obj_t self_in) {
    papers3_rtc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    // ESP_LOGI(TAG, "初始化BM8563 RTC");  // 移除可能阻塞启动的日志
    
    // 初始化I2C总线（如果尚未初始化）
    esp_err_t ret = papers3_i2c_init();
    if (ret != ESP_OK) {
        // ESP_LOGE(TAG, "I2C初始化失败: %s", esp_err_to_name(ret));  // 移除可能阻塞启动的日志
        mp_raise_OSError(MP_EIO);
    }
    
    // 检查RTC是否响应
    uint8_t status1;
    ret = bm8563_i2c_read(BM8563_CONTROL_STATUS1, &status1, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "无法读取BM8563状态寄存器: %s", esp_err_to_name(ret));
        mp_raise_OSError(MP_EIO);
    }
    
    // 清除控制/状态寄存器
    ret = bm8563_i2c_write(BM8563_CONTROL_STATUS1, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "清除状态寄存器1失败: %s", esp_err_to_name(ret));
        mp_raise_OSError(MP_EIO);
    }
    
    ret = bm8563_i2c_write(BM8563_CONTROL_STATUS2, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "清除状态寄存器2失败: %s", esp_err_to_name(ret));
        mp_raise_OSError(MP_EIO);
    }
    
    self->initialized = true;
    ESP_LOGI(TAG, "BM8563 RTC初始化成功，状态1: 0x%02X", status1);
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_rtc_init_obj, papers3_rtc_init);

// deinit() 方法
static mp_obj_t papers3_rtc_deinit(mp_obj_t self_in) {
    papers3_rtc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
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
        esp_err_t ret = bm8563_i2c_read(BM8563_SECONDS, rtc_data, 7);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "读取RTC时间失败: %s", esp_err_to_name(ret));
            mp_raise_OSError(MP_EIO);
        }
        
        // 转换BCD到十进制
        uint8_t second = bcd_to_dec(rtc_data[0] & 0x7F);
        uint8_t minute = bcd_to_dec(rtc_data[1] & 0x7F);
        uint8_t hour = bcd_to_dec(rtc_data[2] & 0x3F);
        uint8_t day = bcd_to_dec(rtc_data[3] & 0x3F);
        uint8_t weekday = bcd_to_dec(rtc_data[4] & 0x07);
        uint8_t month = bcd_to_dec(rtc_data[5] & 0x1F);
        uint8_t year = bcd_to_dec(rtc_data[6]) + 2000;  // BM8563从2000年开始计算
        
        mp_obj_t tuple[7] = {
            mp_obj_new_int(year),
            mp_obj_new_int(month),
            mp_obj_new_int(day),
            mp_obj_new_int(weekday),
            mp_obj_new_int(hour),
            mp_obj_new_int(minute),
            mp_obj_new_int(second)
        };
        
        return mp_obj_new_tuple(7, tuple);
    } else {
        // 设置时间
        if (n_args != 8) {
            mp_raise_TypeError(MP_ERROR_TEXT("datetime requires 7 arguments: (year, month, day, weekday, hour, minute, second)"));
        }
        
        uint16_t year = mp_obj_get_int(args[1]);
        uint8_t month = mp_obj_get_int(args[2]);
        uint8_t day = mp_obj_get_int(args[3]);
        uint8_t weekday = mp_obj_get_int(args[4]);
        uint8_t hour = mp_obj_get_int(args[5]);
        uint8_t minute = mp_obj_get_int(args[6]);
        uint8_t second = mp_obj_get_int(args[7]);
        
        // 验证输入范围
        if (year < 2000 || year > 2099 || month < 1 || month > 12 || 
            day < 1 || day > 31 || weekday > 6 || 
            hour > 23 || minute > 59 || second > 59) {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid datetime values"));
        }
        
        // 转换为BCD格式并写入RTC
        uint8_t rtc_data[7] = {
            dec_to_bcd(second),
            dec_to_bcd(minute),
            dec_to_bcd(hour),
            dec_to_bcd(day),
            dec_to_bcd(weekday),
            dec_to_bcd(month),
            dec_to_bcd(year - 2000)  // BM8563从2000年开始计算
        };
        
        esp_err_t ret = ESP_OK;
        for (int i = 0; i < 7; i++) {
            ret = bm8563_i2c_write(BM8563_SECONDS + i, rtc_data[i]);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "设置RTC时间失败: %s", esp_err_to_name(ret));
                mp_raise_OSError(MP_EIO);
            }
        }
        
        ESP_LOGI(TAG, "RTC时间设置成功: %04d-%02d-%02d %02d:%02d:%02d", 
                 year, month, day, hour, minute, second);
        
        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_rtc_datetime_obj, 1, 8, papers3_rtc_datetime);

// alarm() 方法 - 设置或读取闹钟
static mp_obj_t papers3_rtc_alarm(size_t n_args, const mp_obj_t *args) {
    papers3_rtc_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_ValueError(MP_ERROR_TEXT("RTC not initialized"));
    }
    
    if (n_args == 1) {
        // 读取闹钟设置
        uint8_t alarm_data[4];
        esp_err_t ret = bm8563_i2c_read(BM8563_MINUTE_ALARM, alarm_data, 4);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "读取闹钟设置失败: %s", esp_err_to_name(ret));
            mp_raise_OSError(MP_EIO);
        }
        
        // 检查闹钟是否启用
        bool enabled = !(alarm_data[0] & BM8563_ALARM_ENABLE);
        
        if (enabled) {
            uint8_t minute = bcd_to_dec(alarm_data[0] & 0x7F);
            uint8_t hour = bcd_to_dec(alarm_data[1] & 0x3F);
            uint8_t day = bcd_to_dec(alarm_data[2] & 0x3F);
            uint8_t weekday = bcd_to_dec(alarm_data[3] & 0x07);
            
            mp_obj_t tuple[4] = {
                mp_obj_new_int(hour),
                mp_obj_new_int(minute),
                mp_obj_new_int(day),
                mp_obj_new_int(weekday)
            };
            
            return mp_obj_new_tuple(4, tuple);
        } else {
            return mp_const_none;  // 闹钟未启用
        }
    } else {
        // 设置闹钟
        if (n_args != 3) {
            mp_raise_TypeError(MP_ERROR_TEXT("alarm requires 2 arguments: (hour, minute) or None to disable"));
        }
        
        if (args[1] == mp_const_none) {
            // 禁用闹钟
            esp_err_t ret = bm8563_i2c_write(BM8563_MINUTE_ALARM, BM8563_ALARM_ENABLE);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "禁用闹钟失败: %s", esp_err_to_name(ret));
                mp_raise_OSError(MP_EIO);
            }
            ESP_LOGI(TAG, "闹钟已禁用");
        } else {
            // 启用闹钟
            uint8_t hour = mp_obj_get_int(args[1]);
            uint8_t minute = mp_obj_get_int(args[2]);
            
            if (hour > 23 || minute > 59) {
                mp_raise_ValueError(MP_ERROR_TEXT("Invalid alarm time"));
            }
            
            // 设置闹钟（只设置时和分）
            esp_err_t ret = bm8563_i2c_write(BM8563_MINUTE_ALARM, dec_to_bcd(minute));
            if (ret == ESP_OK) {
                ret = bm8563_i2c_write(BM8563_HOUR_ALARM, dec_to_bcd(hour));
            }
            if (ret == ESP_OK) {
                ret = bm8563_i2c_write(BM8563_DAY_ALARM, BM8563_ALARM_ENABLE);    // 禁用日闹钟
            }
            if (ret == ESP_OK) {
                ret = bm8563_i2c_write(BM8563_WEEKDAY_ALARM, BM8563_ALARM_ENABLE); // 禁用周闹钟
            }
            
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "设置闹钟失败: %s", esp_err_to_name(ret));
                mp_raise_OSError(MP_EIO);
            }
            
            ESP_LOGI(TAG, "闹钟设置成功: %02d:%02d", hour, minute);
        }
        
        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_rtc_alarm_obj, 1, 3, papers3_rtc_alarm);

// 本地方法表
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