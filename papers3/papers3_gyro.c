/*
 * Papers3 BMI270 Gyroscope Module
 * Refactored to use shared I2C manager
 */

#include "py/runtime.h"
#include "py/obj.h"
#include "py/builtin.h"
#include "py/mperrno.h"

#include "papers3_pins.h"
#include "papers3_i2c.h"
#include "esp_log.h"

#define TAG "papers3_gyro"

// BMI270 Config
#define BMI270_ADDR             0x68
#define BMI270_CHIP_ID_ADDR     0x00
#define BMI270_ACC_X_LSB_ADDR   0x0C
#define BMI270_GYR_X_LSB_ADDR   0x12
#define BMI270_PWR_CTRL_ADDR    0x7D
#define BMI270_ACC_CONF_ADDR    0x40
#define BMI270_GYR_CONF_ADDR    0x42

#define BMI270_CHIP_ID          0x24

#define BMI270_ACC_RANGE_4G     8192.0f
#define BMI270_GYR_RANGE_2000   16.384f

typedef struct _papers3_gyro_obj_t {
    mp_obj_base_t base;
    bool initialized;
} papers3_gyro_obj_t;

static mp_obj_t papers3_gyro_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    extern const mp_obj_type_t papers3_gyro_type;
    papers3_gyro_obj_t *self = mp_obj_malloc(papers3_gyro_obj_t, &papers3_gyro_type);
    self->initialized = false;
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t papers3_gyro_init(mp_obj_t self_in) {
    papers3_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    // Init Shared I2C
    if (papers3_i2c_init() != ESP_OK) {
        mp_raise_OSError(MP_EIO);
    }
    
    // Check Chip ID
    uint8_t chip_id;
    if (papers3_i2c_read(BMI270_ADDR, BMI270_CHIP_ID_ADDR, &chip_id, 1) != ESP_OK) {
        papers3_i2c_deinit();
        mp_raise_OSError(MP_EIO);
    }
    
    if (chip_id != BMI270_CHIP_ID) {
        ESP_LOGE(TAG, "BMI270 ID mismatch: 0x%02X", chip_id);
        papers3_i2c_deinit();
        mp_raise_OSError(MP_ENODEV);
    }
    
    // Enable sensors
    papers3_i2c_write(BMI270_ADDR, BMI270_PWR_CTRL_ADDR, 0x06); // ACC+GYR enable
    // Config ACC
    papers3_i2c_write(BMI270_ADDR, BMI270_ACC_CONF_ADDR, 0x28); 
    // Config GYR
    papers3_i2c_write(BMI270_ADDR, BMI270_GYR_CONF_ADDR, 0x28);
    
    self->initialized = true;
    ESP_LOGI(TAG, "BMI270 initialized");
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_gyro_init_obj, papers3_gyro_init);

static mp_obj_t papers3_gyro_deinit(mp_obj_t self_in) {
    papers3_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->initialized) {
        // Disable sensors
        papers3_i2c_write(BMI270_ADDR, BMI270_PWR_CTRL_ADDR, 0x00);
        
        // Deinit Shared I2C
        papers3_i2c_deinit();
        
        self->initialized = false;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_gyro_deinit_obj, papers3_gyro_deinit);

static mp_obj_t papers3_gyro_read_accel(mp_obj_t self_in) {
    papers3_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->initialized) mp_raise_ValueError(MP_ERROR_TEXT("Not initialized"));
    
    uint8_t data[6];
    if (papers3_i2c_read(BMI270_ADDR, BMI270_ACC_X_LSB_ADDR, data, 6) != ESP_OK) {
        mp_raise_OSError(MP_EIO);
    }
    
    int16_t acc_x = (int16_t)((data[1] << 8) | data[0]);
    int16_t acc_y = (int16_t)((data[3] << 8) | data[2]);
    int16_t acc_z = (int16_t)((data[5] << 8) | data[4]);
    
    mp_obj_t tuple[3] = {
        mp_obj_new_float(acc_x / BMI270_ACC_RANGE_4G),
        mp_obj_new_float(acc_y / BMI270_ACC_RANGE_4G),
        mp_obj_new_float(acc_z / BMI270_ACC_RANGE_4G)
    };
    return mp_obj_new_tuple(3, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_gyro_read_accel_obj, papers3_gyro_read_accel);

static mp_obj_t papers3_gyro_read_gyro(mp_obj_t self_in) {
    papers3_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->initialized) mp_raise_ValueError(MP_ERROR_TEXT("Not initialized"));
    
    uint8_t data[6];
    if (papers3_i2c_read(BMI270_ADDR, BMI270_GYR_X_LSB_ADDR, data, 6) != ESP_OK) {
        mp_raise_OSError(MP_EIO);
    }
    
    int16_t gyr_x = (int16_t)((data[1] << 8) | data[0]);
    int16_t gyr_y = (int16_t)((data[3] << 8) | data[2]);
    int16_t gyr_z = (int16_t)((data[5] << 8) | data[4]);
    
    mp_obj_t tuple[3] = {
        mp_obj_new_float(gyr_x / BMI270_GYR_RANGE_2000),
        mp_obj_new_float(gyr_y / BMI270_GYR_RANGE_2000),
        mp_obj_new_float(gyr_z / BMI270_GYR_RANGE_2000)
    };
    return mp_obj_new_tuple(3, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_gyro_read_gyro_obj, papers3_gyro_read_gyro);

static const mp_rom_map_elem_t papers3_gyro_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_gyro_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_gyro_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_accel), MP_ROM_PTR(&papers3_gyro_read_accel_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_gyro), MP_ROM_PTR(&papers3_gyro_read_gyro_obj) },
};
static MP_DEFINE_CONST_DICT(papers3_gyro_locals_dict, papers3_gyro_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    papers3_gyro_type,
    MP_QSTR_Gyro,
    MP_TYPE_FLAG_NONE,
    make_new, papers3_gyro_make_new,
    locals_dict, &papers3_gyro_locals_dict
);