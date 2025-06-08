/*
 * Papers3 BMI270 陀螺仪模块
 * 
 * 硬件: BMI270 (I2C地址: 0x68)
 * 功能: 三轴加速度计 + 三轴陀螺仪
 * 设计: 面向对象接口 papers3.Gyro()
 * 接口: 使用MicroPython的machine.I2C
 */

#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"
#include "py/mperrno.h"

#include "esp_log.h"
#include "py/builtin.h"

#define TAG "papers3_gyro"

// BMI270 I2C配置 - 使用MicroPython的I2C接口
#define BMI270_I2C_ADDR         0x68
#define BMI270_I2C_SDA_PIN      8
#define BMI270_I2C_SCL_PIN      9

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

// 结构体定义
typedef struct _papers3_gyro_obj_t {
    mp_obj_base_t base;
    bool initialized;
    mp_obj_t i2c_obj;  // MicroPython I2C对象
} papers3_gyro_obj_t;

// I2C读写函数 - 使用MicroPython的I2C接口
static int bmi270_i2c_read(papers3_gyro_obj_t *self, uint8_t reg_addr, uint8_t *data, size_t len) {
    if (!self->i2c_obj || self->i2c_obj == mp_const_none) {
        return -1;
    }
    
    mp_obj_t dest[3];
    
    // 准备参数: addr, memaddr, nbytes
    dest[0] = mp_obj_new_int(BMI270_I2C_ADDR);
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

static int bmi270_i2c_write(papers3_gyro_obj_t *self, uint8_t reg_addr, uint8_t data) {
    if (!self->i2c_obj || self->i2c_obj == mp_const_none) {
        return -1;
    }
    
    mp_obj_t dest[3];
    
    // 准备参数: addr, memaddr, data
    dest[0] = mp_obj_new_int(BMI270_I2C_ADDR);
    dest[1] = mp_obj_new_int(reg_addr);
    dest[2] = mp_obj_new_bytes(&data, 1);
    
    // 调用 i2c.writeto_mem(addr, memaddr, data)
    mp_obj_t args[5] = {self->i2c_obj, MP_OBJ_NEW_QSTR(MP_QSTR_writeto_mem), dest[0], dest[1], dest[2]};
    mp_obj_t result = mp_call_method_n_kw(3, 0, args);
    
    return (result != mp_const_none) ? 0 : -1;
}

// Gyro构造函数
static mp_obj_t papers3_gyro_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    
    // 前向声明类型
    extern const mp_obj_type_t papers3_gyro_type;
    papers3_gyro_obj_t *self = mp_obj_malloc(papers3_gyro_obj_t, &papers3_gyro_type);
    self->initialized = false;
    self->i2c_obj = mp_const_none;
    
    return MP_OBJ_FROM_PTR(self);
}

// init() 方法
static mp_obj_t papers3_gyro_init(mp_obj_t self_in) {
    papers3_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    ESP_LOGI(TAG, "初始化BMI270陀螺仪");
    
    // 创建I2C对象 machine.I2C(0, scl=Pin(9), sda=Pin(8), freq=100000)
    mp_obj_t args[5];
    
    // 导入machine模块
    mp_obj_t machine_module = mp_import_name(MP_QSTR_machine, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
    
    // 获取Pin和I2C类
    mp_obj_t pin_class = mp_load_attr(machine_module, MP_QSTR_Pin);
    mp_obj_t i2c_class = mp_load_attr(machine_module, MP_QSTR_I2C);
    
    // 创建Pin对象
    mp_obj_t scl_pin = mp_call_function_1(pin_class, mp_obj_new_int(BMI270_I2C_SCL_PIN));
    mp_obj_t sda_pin = mp_call_function_1(pin_class, mp_obj_new_int(BMI270_I2C_SDA_PIN));
    
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
    
    // 检查芯片ID
    uint8_t chip_id;
    if (bmi270_i2c_read(self, BMI270_CHIP_ID_ADDR, &chip_id, 1) != 0) {
        ESP_LOGE(TAG, "无法读取BMI270芯片ID");
        mp_raise_OSError(MP_EIO);
    }
    
    if (chip_id != BMI270_CHIP_ID) {
        ESP_LOGE(TAG, "BMI270芯片ID错误: 0x%02X (期望: 0x%02X)", chip_id, BMI270_CHIP_ID);
        mp_raise_OSError(MP_ENODEV);
    }
    
    // 启用加速度计和陀螺仪
    if (bmi270_i2c_write(self, BMI270_PWR_CTRL_ADDR, BMI270_ACC_EN | BMI270_GYR_EN) != 0) {
        mp_raise_OSError(MP_EIO);
    }
    
    // 配置加速度计 (100Hz, ±4G)
    if (bmi270_i2c_write(self, BMI270_ACC_CONF_ADDR, 0x28) != 0) {
        mp_raise_OSError(MP_EIO);
    }
    
    // 配置陀螺仪 (100Hz, ±2000dps)
    if (bmi270_i2c_write(self, BMI270_GYR_CONF_ADDR, 0x28) != 0) {
        mp_raise_OSError(MP_EIO);
    }
    
    self->initialized = true;
    ESP_LOGI(TAG, "BMI270初始化成功");
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_gyro_init_obj, papers3_gyro_init);

// deinit() 方法
static mp_obj_t papers3_gyro_deinit(mp_obj_t self_in) {
    papers3_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        // 禁用传感器
        bmi270_i2c_write(self, BMI270_PWR_CTRL_ADDR, 0x00);
        self->i2c_obj = mp_const_none;
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
    if (bmi270_i2c_read(self, BMI270_ACC_X_LSB_ADDR, data, 6) != 0) {
        mp_raise_OSError(MP_EIO);
    }
    
    // 组合16位数据
    int16_t raw_x = (int16_t)((data[1] << 8) | data[0]);
    int16_t raw_y = (int16_t)((data[3] << 8) | data[2]);
    int16_t raw_z = (int16_t)((data[5] << 8) | data[4]);
    
    // 转换为g单位
    float acc_x = raw_x / BMI270_ACC_RANGE_4G;
    float acc_y = raw_y / BMI270_ACC_RANGE_4G;
    float acc_z = raw_z / BMI270_ACC_RANGE_4G;
    
    mp_obj_t tuple[3] = {
        mp_obj_new_float(acc_x),
        mp_obj_new_float(acc_y),
        mp_obj_new_float(acc_z)
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
    if (bmi270_i2c_read(self, BMI270_GYR_X_LSB_ADDR, data, 6) != 0) {
        mp_raise_OSError(MP_EIO);
    }
    
    // 组合16位数据
    int16_t raw_x = (int16_t)((data[1] << 8) | data[0]);
    int16_t raw_y = (int16_t)((data[3] << 8) | data[2]);
    int16_t raw_z = (int16_t)((data[5] << 8) | data[4]);
    
    // 转换为dps单位
    float gyro_x = raw_x / BMI270_GYR_RANGE_2000;
    float gyro_y = raw_y / BMI270_GYR_RANGE_2000;
    float gyro_z = raw_z / BMI270_GYR_RANGE_2000;
    
    mp_obj_t tuple[3] = {
        mp_obj_new_float(gyro_x),
        mp_obj_new_float(gyro_y), 
        mp_obj_new_float(gyro_z)
    };
    
    return mp_obj_new_tuple(3, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_gyro_read_gyro_obj, papers3_gyro_read_gyro);

// 方法表
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