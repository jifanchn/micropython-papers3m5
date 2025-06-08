#include "py/runtime.h"
#include "py/obj.h"
#include "py/objint.h"

// ESP-IDF includes
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char* TAG = "papers3_touch";

// GT911配置 (参考用户提供的测试代码)
#define GT911_SDA_PIN     GPIO_NUM_41  // SDA引脚
#define GT911_SCL_PIN     GPIO_NUM_42  // SCL引脚
#define GT911_INT_PIN     GPIO_NUM_48  // 中断引脚
#define GT911_I2C_PORT    I2C_NUM_0    // I2C端口
#define GT911_I2C_FREQ    100000       // I2C频率

// GT911寄存器地址
#define GT911_ADDR_1      0x14         // GT911地址1
#define GT911_ADDR_2      0x5D         // GT911地址2
#define GT911_STATUS_REG  0x814E       // 状态寄存器
#define GT911_COORD_REG   0x814F       // 坐标寄存器

// 触摸点数据结构
typedef struct {
    int x;
    int y;
    int size;
    int id;
} touch_point_t;

// GT911对象类型定义
typedef struct _papers3_touch_obj_t {
    mp_obj_base_t base;
    bool initialized;
    uint8_t gt911_addr;               // GT911设备地址
    bool gt911_irq_trigger;           // 中断触发标志
    touch_point_t fingers[2];         // 最多2个触摸点
    int num_touches;                  // 当前触摸点数量
    bool is_finger_up;                // 手指抬起标志
    int rotate;                       // 旋转模式 (1=90度)
} papers3_touch_obj_t;

// 中断处理函数
static papers3_touch_obj_t *touch_instance = NULL;

static void IRAM_ATTR gt911_irq_handler(void *arg) {
    if (touch_instance) {
        touch_instance->gt911_irq_trigger = true;
    }
}

// GT911类构造函数
static mp_obj_t papers3_touch_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    
    papers3_touch_obj_t *self = mp_obj_malloc(papers3_touch_obj_t, type);
    self->initialized = false;
    self->gt911_addr = 0;
    self->gt911_irq_trigger = false;
    self->num_touches = 0;
    self->is_finger_up = false;
    self->rotate = 1;  // 默认90度旋转
    
    // 初始化触摸点数据
    for (int i = 0; i < 2; i++) {
        self->fingers[i].x = 0;
        self->fingers[i].y = 0;
        self->fingers[i].size = 0;
        self->fingers[i].id = 0;
    }
    
    return MP_OBJ_FROM_PTR(self);
}

// I2C写入函数
static bool gt911_write(papers3_touch_obj_t *self, uint16_t addr, uint8_t data) {
    if (self->gt911_addr == 0) {
        return false;
    }
    
    uint8_t write_data[3] = {
        (addr >> 8) & 0xFF,  // 地址高字节
        addr & 0xFF,         // 地址低字节
        data                 // 数据
    };
    
    esp_err_t ret = i2c_master_write_to_device(GT911_I2C_PORT, self->gt911_addr, 
                                               write_data, sizeof(write_data), 
                                               pdMS_TO_TICKS(100));
    return (ret == ESP_OK);
}

// I2C读取函数
static bool gt911_read(papers3_touch_obj_t *self, uint16_t addr, uint8_t *data, size_t length) {
    if (self->gt911_addr == 0) {
        return false;
    }
    
    uint8_t reg_addr[2] = {
        (addr >> 8) & 0xFF,  // 地址高字节
        addr & 0xFF          // 地址低字节
    };
    
    esp_err_t ret = i2c_master_write_read_device(GT911_I2C_PORT, self->gt911_addr,
                                                 reg_addr, sizeof(reg_addr),
                                                 data, length,
                                                 pdMS_TO_TICKS(100));
    return (ret == ESP_OK);
}

// 检测GT911地址 (模拟Arduino Wire.beginTransmission方式)
static uint8_t detect_gt911_address(void) {
    esp_err_t ret;
    
    // 先尝试地址0x14 (模拟Wire.beginTransmission + endTransmission)
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (GT911_ADDR_1 << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(GT911_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    if (ret == ESP_OK) {
        return GT911_ADDR_1;
    }
    
    // 再尝试地址0x5D
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (GT911_ADDR_2 << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(GT911_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    if (ret == ESP_OK) {
        return GT911_ADDR_2;
    }
    
    return 0;  // 未找到设备
}

// 外部I2C初始化标志 (与gyro模块共享)
extern bool g_i2c_initialized;

// I2C初始化函数 (共享机制，不重复配置)
static esp_err_t papers3_touch_i2c_init(void) {
    if (g_i2c_initialized) {
        return ESP_OK;  // 已经初始化，直接返回
    }
    
    // 如果I2C还未初始化，使用与gyro模块相同的配置
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GT911_SDA_PIN,
        .scl_io_num = GT911_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,  // 使用标准100kHz，与gyro模块一致
        .clk_flags = 0,
    };
    
    esp_err_t ret = i2c_param_config(GT911_I2C_PORT, &conf);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = i2c_driver_install(GT911_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        return ret;
    }
    
    g_i2c_initialized = true;
    // 移除可能阻塞的日志: ESP_LOGI(TAG, "I2C总线初始化成功");
    return ESP_OK;
}

// 初始化GT911 (参考Arduino代码的初始化顺序)
static mp_obj_t papers3_touch_init(mp_obj_t self_in) {
    papers3_touch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        return mp_const_true;
    }
    
    // 1. 先配置中断引脚为输入 (参考Arduino pinMode(pin_int, INPUT))
    gpio_config_t int_conf = {
        .pin_bit_mask = (1ULL << GT911_INT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,  // 先不启用中断
    };
    
    esp_err_t ret = gpio_config(&int_conf);
    if (ret != ESP_OK) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("GT911 INT GPIO config failed"));
    }
    
    // 2. 初始化I2C总线 (参考Arduino Wire.begin)
    ret = papers3_touch_i2c_init();
    if (ret != ESP_OK) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("I2C init failed"));
    }
    
    // 3. 延时100ms (参考Arduino delay(100))
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 4. 检测GT911地址
    self->gt911_addr = detect_gt911_address();
    if (self->gt911_addr == 0) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("GT911 not found"));
    }
    
    // 5. 安装中断服务 (参考Arduino attachInterrupt)
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("GPIO ISR service install failed"));
    }
    
    // 6. 注册中断处理函数 (FALLING edge, 参考Arduino FALLING)
    touch_instance = self;
    ret = gpio_isr_handler_add(GT911_INT_PIN, gt911_irq_handler, NULL);
    if (ret != ESP_OK) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("GT911 IRQ handler add failed"));
    }
    
    // 7. 启用中断 (现在设备已经检测到，可以安全启用中断)
    ret = gpio_set_intr_type(GT911_INT_PIN, GPIO_INTR_NEGEDGE);
    if (ret != ESP_OK) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("GT911 interrupt enable failed"));
    }
    
    self->initialized = true;
    
    // 移除可能阻塞的输出: mp_printf(&mp_plat_print, "GT911 initialized (addr=0x%02X, SDA=%d, SCL=%d, INT=%d)\n", 
    //          self->gt911_addr, GT911_SDA_PIN, GT911_SCL_PIN, GT911_INT_PIN);
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_touch_init_obj, papers3_touch_init);

// 检查是否有新的触摸数据
static mp_obj_t papers3_touch_available(mp_obj_t self_in) {
    papers3_touch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Touch not initialized"));
    }
    
    if (self->gt911_irq_trigger) {
        self->gt911_irq_trigger = false;
        return mp_const_true;
    }
    
    return mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_touch_available_obj, papers3_touch_available);

// 清除GT911状态
static mp_obj_t papers3_touch_flush(mp_obj_t self_in) {
    papers3_touch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Touch not initialized"));
    }
    
    gt911_write(self, GT911_STATUS_REG, 0x00);
    self->gt911_irq_trigger = false;
    self->num_touches = 0;
    self->is_finger_up = false;
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_touch_flush_obj, papers3_touch_flush);

// 更新触摸数据
static mp_obj_t papers3_touch_update(mp_obj_t self_in) {
    papers3_touch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Touch not initialized"));
    }
    
    uint8_t status;
    if (!gt911_read(self, GT911_STATUS_REG, &status, 1)) {
        return mp_const_false;
    }
    
    int num = status & 0x0F;
    
    if (status & 0x80) {  // 有新的触摸数据
        if (num != 0) {
            self->is_finger_up = false;
            self->num_touches = num;
            
            // 读取触摸坐标数据
            uint8_t data[16];  // 最多2个触摸点，每个8字节
            if (gt911_read(self, GT911_COORD_REG, data, num * 8)) {
                for (int j = 0; j < (num < 2 ? num : 2); j++) {
                    uint8_t *buf = &data[j * 8];
                    
                    // 根据旋转模式处理坐标
                    if (self->rotate == 1) {  // ROTATE_90 (默认)
                        self->fingers[j].x = (buf[1] << 8) | buf[0];
                        self->fingers[j].y = (buf[3] << 8) | buf[2];
                    } else {
                        // 其他旋转模式可以后续添加
                        self->fingers[j].x = (buf[1] << 8) | buf[0];
                        self->fingers[j].y = (buf[3] << 8) | buf[2];
                    }
                    
                    self->fingers[j].size = (buf[5] << 8) | buf[4];
                    self->fingers[j].id = buf[7];
                }
            }
        } else {
            self->is_finger_up = true;
        }
        
        // 清除状态寄存器
        gt911_write(self, GT911_STATUS_REG, 0x00);
    } else {
        self->is_finger_up = true;
    }
    
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_touch_update_obj, papers3_touch_update);

// 获取触摸点数量
static mp_obj_t papers3_touch_get_touches(mp_obj_t self_in) {
    papers3_touch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Touch not initialized"));
    }
    
    return mp_obj_new_int(self->num_touches);
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_touch_get_touches_obj, papers3_touch_get_touches);

// 获取指定触摸点的坐标
static mp_obj_t papers3_touch_get_point(mp_obj_t self_in, mp_obj_t index_obj) {
    papers3_touch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Touch not initialized"));
    }
    
    int index = mp_obj_get_int(index_obj);
    if (index < 0 || index >= 2) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Touch point index out of range"));
    }
    
    // 返回元组 (x, y, size, id)
    mp_obj_t tuple[4] = {
        mp_obj_new_int(self->fingers[index].x),
        mp_obj_new_int(self->fingers[index].y),
        mp_obj_new_int(self->fingers[index].size),
        mp_obj_new_int(self->fingers[index].id),
    };
    
    return mp_obj_new_tuple(4, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_2(papers3_touch_get_point_obj, papers3_touch_get_point);

// 反初始化触摸屏
static mp_obj_t papers3_touch_deinit(mp_obj_t self_in) {
    papers3_touch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        // 移除中断处理函数
        gpio_isr_handler_remove(GT911_INT_PIN);
        touch_instance = NULL;
        
        // 卸载I2C驱动
        i2c_driver_delete(GT911_I2C_PORT);
        
        self->initialized = false;
        mp_printf(&mp_plat_print, "GT911 deinitialized\n");
    }
    
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_touch_deinit_obj, papers3_touch_deinit);

// 触摸屏类方法表
static const mp_rom_map_elem_t papers3_touch_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_touch_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_available), MP_ROM_PTR(&papers3_touch_available_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&papers3_touch_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&papers3_touch_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_touches), MP_ROM_PTR(&papers3_touch_get_touches_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_point), MP_ROM_PTR(&papers3_touch_get_point_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_touch_deinit_obj) },
};
static MP_DEFINE_CONST_DICT(papers3_touch_locals_dict, papers3_touch_locals_dict_table);

// 触摸屏类型定义
MP_DEFINE_CONST_OBJ_TYPE(
    papers3_touch_type,
    MP_QSTR_Touch,
    MP_TYPE_FLAG_NONE,
    make_new, papers3_touch_make_new,
    locals_dict, &papers3_touch_locals_dict
); 