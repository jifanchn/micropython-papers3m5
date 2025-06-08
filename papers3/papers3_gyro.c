/*
 * Papers3 BMI270 Gyroscope Module
 * 使用ESP-IDF I2C驱动，避免重复初始化冲突
 */

#include "py/runtime.h"
#include "py/obj.h"
#include "py/builtin.h"
#include "py/mperrno.h"

// ESP-IDF I2C驱动
#include "driver/i2c.h"
#include "esp_log.h"

#define TAG "papers3_gyro"

// BMI270 硬件配置
#define BMI270_I2C_ADDR         0x68
#define BMI270_I2C_PORT         I2C_NUM_0
#define BMI270_I2C_SDA_PIN      41
#define BMI270_I2C_SCL_PIN      42
#define BMI270_I2C_FREQ         100000

// BMI270 寄存器地址
#define BMI270_CHIP_ID_ADDR     0x00
#define BMI270_ACC_X_LSB_ADDR   0x0C
#define BMI270_GYR_X_LSB_ADDR   0x12
#define BMI270_PWR_CTRL_ADDR    0x7D
#define BMI270_ACC_CONF_ADDR    0x40
#define BMI270_GYR_CONF_ADDR    0x42

// BMI270 芯片ID
#define BMI270_CHIP_ID          0x24

// BMI270 电源控制
#define BMI270_ACC_EN           0x04
#define BMI270_GYR_EN           0x02

// BMI270 数据转换系数
#define BMI270_ACC_RANGE_4G     8192.0f   // ±4G range
#define BMI270_GYR_RANGE_2000   16.384f   // ±2000dps range

// 全局I2C初始化标志
bool g_i2c_initialized = false;

// 结构体定义
typedef struct _papers3_gyro_obj_t {
    mp_obj_base_t base;
    bool initialized;
} papers3_gyro_obj_t;

// ESP-IDF I2C初始化函数
static esp_err_t papers3_i2c_init(void) {
    if (g_i2c_initialized) {
        return ESP_OK;  // 已经初始化，直接返回
    }
    
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BMI270_I2C_SDA_PIN,
        .scl_io_num = BMI270_I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BMI270_I2C_FREQ,
    };
    
    esp_err_t ret = i2c_param_config(BMI270_I2C_PORT, &conf);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = i2c_driver_install(BMI270_I2C_PORT, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        return ret;
    }
    
    g_i2c_initialized = true;
    ESP_LOGI(TAG, "I2C总线初始化成功");
    return ESP_OK;
}

// I2C读写函数
static esp_err_t bmi270_i2c_read(uint8_t reg_addr, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BMI270_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BMI270_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(BMI270_I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t bmi270_i2c_write(uint8_t reg_addr, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BMI270_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(BMI270_I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

// Gyro构造函数
static mp_obj_t papers3_gyro_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    
    extern const mp_obj_type_t papers3_gyro_type;
    papers3_gyro_obj_t *self = mp_obj_malloc(papers3_gyro_obj_t, &papers3_gyro_type);
    self->initialized = false;
    
    return MP_OBJ_FROM_PTR(self);
}

// init() 方法
static mp_obj_t papers3_gyro_init(mp_obj_t self_in) {
    papers3_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    ESP_LOGI(TAG, "初始化BMI270陀螺仪");
    
    // 初始化I2C总线
    esp_err_t ret = papers3_i2c_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C初始化失败: %s", esp_err_to_name(ret));
        mp_raise_OSError(MP_EIO);
    }
    
    // 检查芯片ID
    uint8_t chip_id;
    ret = bmi270_i2c_read(BMI270_CHIP_ID_ADDR, &chip_id, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "无法读取BMI270芯片ID: %s", esp_err_to_name(ret));
        mp_raise_OSError(MP_EIO);
    }
    
    if (chip_id != BMI270_CHIP_ID) {
        ESP_LOGE(TAG, "BMI270芯片ID错误: 0x%02X (期望: 0x%02X)", chip_id, BMI270_CHIP_ID);
        mp_raise_OSError(MP_ENODEV);
    }
    
    // 启用加速度计和陀螺仪
    ret = bmi270_i2c_write(BMI270_PWR_CTRL_ADDR, BMI270_ACC_EN | BMI270_GYR_EN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "启用传感器失败: %s", esp_err_to_name(ret));
        mp_raise_OSError(MP_EIO);
    }
    
    // 配置加速度计 (100Hz, ±4G)
    ret = bmi270_i2c_write(BMI270_ACC_CONF_ADDR, 0x28);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "配置加速度计失败: %s", esp_err_to_name(ret));
        mp_raise_OSError(MP_EIO);
    }
    
    // 配置陀螺仪 (100Hz, ±2000dps)
    ret = bmi270_i2c_write(BMI270_GYR_CONF_ADDR, 0x28);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "配置陀螺仪失败: %s", esp_err_to_name(ret));
        mp_raise_OSError(MP_EIO);
    }
    
    self->initialized = true;
    ESP_LOGI(TAG, "BMI270初始化成功，芯片ID: 0x%02X", chip_id);
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_gyro_init_obj, papers3_gyro_init);

// deinit() 方法
static mp_obj_t papers3_gyro_deinit(mp_obj_t self_in) {
    papers3_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        // 禁用传感器
        bmi270_i2c_write(BMI270_PWR_CTRL_ADDR, 0x00);
        self->initialized = false;
        ESP_LOGI(TAG, "BMI270已反初始化");
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_gyro_deinit_obj, papers3_gyro_deinit);

// read_accel() 方法 - 读取加速度计 (g)
static mp_obj_t papers3_gyro_read_accel(mp_obj_t self_in) {
    papers3_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_ValueError(MP_ERROR_TEXT("Gyro not initialized"));
    }
    
    uint8_t data[6];
    esp_err_t ret = bmi270_i2c_read(BMI270_ACC_X_LSB_ADDR, data, 6);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取加速度计数据失败: %s", esp_err_to_name(ret));
        mp_raise_OSError(MP_EIO);
    }
    
    // 转换为16位有符号整数
    int16_t acc_x = (int16_t)((data[1] << 8) | data[0]);
    int16_t acc_y = (int16_t)((data[3] << 8) | data[2]);
    int16_t acc_z = (int16_t)((data[5] << 8) | data[4]);
    
    // 转换为g值
    float x = (float)acc_x / BMI270_ACC_RANGE_4G;
    float y = (float)acc_y / BMI270_ACC_RANGE_4G;
    float z = (float)acc_z / BMI270_ACC_RANGE_4G;
    
    mp_obj_t tuple[3] = {
        mp_obj_new_float(x),
        mp_obj_new_float(y),
        mp_obj_new_float(z)
    };
    
    return mp_obj_new_tuple(3, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_gyro_read_accel_obj, papers3_gyro_read_accel);

// read_gyro() 方法 - 读取陀螺仪 (dps)
static mp_obj_t papers3_gyro_read_gyro(mp_obj_t self_in) {
    papers3_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_ValueError(MP_ERROR_TEXT("Gyro not initialized"));
    }
    
    uint8_t data[6];
    esp_err_t ret = bmi270_i2c_read(BMI270_GYR_X_LSB_ADDR, data, 6);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取陀螺仪数据失败: %s", esp_err_to_name(ret));
        mp_raise_OSError(MP_EIO);
    }
    
    // 转换为16位有符号整数
    int16_t gyr_x = (int16_t)((data[1] << 8) | data[0]);
    int16_t gyr_y = (int16_t)((data[3] << 8) | data[2]);
    int16_t gyr_z = (int16_t)((data[5] << 8) | data[4]);
    
    // 转换为度/秒值
    float x = (float)gyr_x / BMI270_GYR_RANGE_2000;
    float y = (float)gyr_y / BMI270_GYR_RANGE_2000;
    float z = (float)gyr_z / BMI270_GYR_RANGE_2000;
    
    mp_obj_t tuple[3] = {
        mp_obj_new_float(x),
        mp_obj_new_float(y),
        mp_obj_new_float(z)
    };
    
    return mp_obj_new_tuple(3, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_gyro_read_gyro_obj, papers3_gyro_read_gyro);

// 本地方法表
static const mp_rom_map_elem_t papers3_gyro_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_gyro_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_gyro_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_accel), MP_ROM_PTR(&papers3_gyro_read_accel_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_gyro), MP_ROM_PTR(&papers3_gyro_read_gyro_obj) },
};
static MP_DEFINE_CONST_DICT(papers3_gyro_locals_dict, papers3_gyro_locals_dict_table);

// 类型定义
MP_DEFINE_CONST_OBJ_TYPE(
    papers3_gyro_type,
    MP_QSTR_Gyro,
    MP_TYPE_FLAG_NONE,
    make_new, papers3_gyro_make_new,
    locals_dict, &papers3_gyro_locals_dict
); 