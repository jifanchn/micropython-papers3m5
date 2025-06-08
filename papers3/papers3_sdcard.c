/*
 * Papers3 SD卡模块
 * 
 * 硬件: SD卡 SPI接口 (MOSI=1, MISO=42, CLK=2, CS=41)
 * 功能: SD卡挂载、文件操作
 * 设计: 面向对象接口 papers3.SDCard()
 * 接口: 使用MicroPython的machine.SPI和os.mount
 */

#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"
#include "py/mperrno.h"

#include "esp_log.h"
#include "py/builtin.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#define TAG "papers3_sdcard"

// SD卡 SPI配置
#define SD_SPI_MOSI_PIN     1
#define SD_SPI_MISO_PIN     42
#define SD_SPI_CLK_PIN      2
#define SD_SPI_CS_PIN       41
#define SD_SPI_FREQ         1000000  // 1MHz

// 结构体定义
typedef struct _papers3_sdcard_obj_t {
    mp_obj_base_t base;
    bool initialized;
    bool mounted;
    mp_obj_t spi_obj;    // MicroPython SPI对象
    mp_obj_t cs_pin_obj; // CS Pin对象
    mp_obj_t vfs_obj;    // VFS对象
} papers3_sdcard_obj_t;

// SD卡构造函数
static mp_obj_t papers3_sdcard_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    
    // 前向声明类型
    extern const mp_obj_type_t papers3_sdcard_type;
    papers3_sdcard_obj_t *self = mp_obj_malloc(papers3_sdcard_obj_t, &papers3_sdcard_type);
    self->initialized = false;
    self->mounted = false;
    self->spi_obj = mp_const_none;
    self->cs_pin_obj = mp_const_none;
    self->vfs_obj = mp_const_none;
    
    return MP_OBJ_FROM_PTR(self);
}

// init() 方法
static mp_obj_t papers3_sdcard_init(mp_obj_t self_in) {
    papers3_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    ESP_LOGI(TAG, "初始化SD卡");
    
    // 导入必要的模块
    mp_obj_t machine_module = mp_import_name(MP_QSTR_machine, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
    
    // 获取Pin和SPI类
    mp_obj_t pin_class = mp_load_attr(machine_module, MP_QSTR_Pin);
    mp_obj_t spi_class = mp_load_attr(machine_module, MP_QSTR_SPI);
    
    // 创建CS Pin对象 (输出，高电平)
    mp_obj_t pin_args[2] = {
        mp_obj_new_int(SD_SPI_CS_PIN),
        mp_load_attr(pin_class, MP_QSTR_OUT)
    };
    self->cs_pin_obj = mp_call_function_n_kw(pin_class, 2, 0, pin_args);
    
    // 设置CS为高电平 (SD卡空闲状态)
    mp_obj_t pin_value_args[1] = {mp_obj_new_int(1)};
    mp_obj_t pin_method_args[3] = {self->cs_pin_obj, MP_OBJ_NEW_QSTR(MP_QSTR_value), pin_value_args[0]};
    mp_call_method_n_kw(1, 0, pin_method_args);
    
    // 创建SPI对象: SPI(1, baudrate=1000000, polarity=0, phase=0, 
    //                  sck=Pin(2), mosi=Pin(1), miso=Pin(42))
    mp_obj_t spi_args[1] = {mp_obj_new_int(1)};  // SPI ID
    
    mp_obj_t sck_pin = mp_call_function_1(pin_class, mp_obj_new_int(SD_SPI_CLK_PIN));
    mp_obj_t mosi_pin = mp_call_function_1(pin_class, mp_obj_new_int(SD_SPI_MOSI_PIN));
    mp_obj_t miso_pin = mp_call_function_1(pin_class, mp_obj_new_int(SD_SPI_MISO_PIN));
    
    mp_obj_t spi_kw_args[6];
    spi_kw_args[0] = mp_obj_new_int(SD_SPI_FREQ);  // baudrate
    spi_kw_args[1] = mp_obj_new_int(0);            // polarity
    spi_kw_args[2] = mp_obj_new_int(0);            // phase
    spi_kw_args[3] = sck_pin;                      // sck
    spi_kw_args[4] = mosi_pin;                     // mosi
    spi_kw_args[5] = miso_pin;                     // miso
    
    static const qstr spi_kw_names[6] = {
        MP_QSTR_baudrate, MP_QSTR_polarity, MP_QSTR_phase,
        MP_QSTR_sck, MP_QSTR_mosi, MP_QSTR_miso
    };
    
    // 构建完整的参数数组 (总共13个参数)
    mp_obj_t all_spi_args[13] = {
        spi_args[0], 
        MP_OBJ_NEW_QSTR(spi_kw_names[0]), spi_kw_args[0],
        MP_OBJ_NEW_QSTR(spi_kw_names[1]), spi_kw_args[1], 
        MP_OBJ_NEW_QSTR(spi_kw_names[2]), spi_kw_args[2],
        MP_OBJ_NEW_QSTR(spi_kw_names[3]), spi_kw_args[3],
        MP_OBJ_NEW_QSTR(spi_kw_names[4]), spi_kw_args[4],
        MP_OBJ_NEW_QSTR(spi_kw_names[5]), spi_kw_args[5]
    };
    self->spi_obj = mp_call_function_n_kw(spi_class, 1, 6, all_spi_args);
    
    // 创建SD卡VFS对象
    mp_obj_t machine_sdcard_module = mp_import_name(MP_QSTR_machine, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
    mp_obj_t sdcard_class = mp_load_attr(machine_sdcard_module, MP_QSTR_SDCard);
    
    // 创建SDCard对象: SDCard(slot=1, sck=Pin(2), mosi=Pin(1), miso=Pin(42), cs=Pin(41))
    mp_obj_t sdcard_args[1] = {mp_obj_new_int(1)};  // slot
    
    mp_obj_t sdcard_kw_args[4];
    sdcard_kw_args[0] = sck_pin;           // sck
    sdcard_kw_args[1] = mosi_pin;          // mosi  
    sdcard_kw_args[2] = miso_pin;          // miso
    sdcard_kw_args[3] = self->cs_pin_obj;  // cs
    
    static const qstr sdcard_kw_names[4] = {
        MP_QSTR_sck, MP_QSTR_mosi, MP_QSTR_miso, MP_QSTR_cs
    };
    
    // 构建完整的参数数组 (总共9个参数)
    mp_obj_t all_sdcard_args[9] = {
        sdcard_args[0],
        MP_OBJ_NEW_QSTR(sdcard_kw_names[0]), sdcard_kw_args[0],
        MP_OBJ_NEW_QSTR(sdcard_kw_names[1]), sdcard_kw_args[1],
        MP_OBJ_NEW_QSTR(sdcard_kw_names[2]), sdcard_kw_args[2],
        MP_OBJ_NEW_QSTR(sdcard_kw_names[3]), sdcard_kw_args[3]
    };
    self->vfs_obj = mp_call_function_n_kw(sdcard_class, 1, 4, all_sdcard_args);
    
    self->initialized = true;
    ESP_LOGI(TAG, "SD卡初始化成功");
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_sdcard_init_obj, papers3_sdcard_init);

// deinit() 方法
static mp_obj_t papers3_sdcard_deinit(mp_obj_t self_in) {
    papers3_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->initialized) {
        // 先卸载 
        if (self->mounted) {
            // 导入os模块进行卸载
            mp_obj_t os_module = mp_import_name(MP_QSTR_os, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
            mp_obj_t umount_args[1] = {mp_obj_new_str("/sd", 3)};
            mp_obj_t umount_method_args[3] = {os_module, MP_OBJ_NEW_QSTR(MP_QSTR_umount), umount_args[0]};
            mp_call_method_n_kw(1, 0, umount_method_args);
            self->mounted = false;
        }
        
        self->spi_obj = mp_const_none;
        self->cs_pin_obj = mp_const_none;
        self->vfs_obj = mp_const_none;
        self->initialized = false;
        ESP_LOGI(TAG, "SD卡已反初始化");
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_sdcard_deinit_obj, papers3_sdcard_deinit);

// mount() 方法 - 挂载SD卡到指定路径 (默认 "/sd")
static mp_obj_t papers3_sdcard_mount(size_t n_args, const mp_obj_t *args) {
    papers3_sdcard_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (!self->initialized) {
        mp_raise_ValueError(MP_ERROR_TEXT("SDCard not initialized"));
    }
    
    if (self->mounted) {
        mp_raise_ValueError(MP_ERROR_TEXT("SDCard already mounted"));
    }
    
    // 获取挂载路径 (默认 "/sd")
    const char *mount_point = "/sd";
    if (n_args > 1) {
        mount_point = mp_obj_str_get_str(args[1]);
    }
    
    // 导入os模块
    mp_obj_t os_module = mp_import_name(MP_QSTR_os, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
    
    // 调用 os.mount(vfs_obj, mount_point)
    mp_obj_t mount_args[2] = {
        self->vfs_obj,
        mp_obj_new_str(mount_point, strlen(mount_point))
    };
    
    mp_obj_t mount_method_args[4] = {os_module, MP_OBJ_NEW_QSTR(MP_QSTR_mount), mount_args[0], mount_args[1]};
    mp_call_method_n_kw(2, 0, mount_method_args);
    
    self->mounted = true;
    ESP_LOGI(TAG, "SD卡已挂载到: %s", mount_point);
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_sdcard_mount_obj, 1, 2, papers3_sdcard_mount);

// unmount() 方法 - 卸载SD卡
static mp_obj_t papers3_sdcard_unmount(mp_obj_t self_in) {
    papers3_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->mounted) {
        return mp_const_none;  // 已经卸载
    }
    
    // 导入os模块
    mp_obj_t os_module = mp_import_name(MP_QSTR_os, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
    
    // 调用 os.umount("/sd")
    mp_obj_t umount_args[1] = {
        mp_obj_new_str("/sd", 3)
    };
    
    mp_obj_t umount_method_args[3] = {os_module, MP_OBJ_NEW_QSTR(MP_QSTR_umount), umount_args[0]};
    mp_call_method_n_kw(1, 0, umount_method_args);
    
    self->mounted = false;
    ESP_LOGI(TAG, "SD卡已卸载");
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_sdcard_unmount_obj, papers3_sdcard_unmount);

// info() 方法 - 获取SD卡信息
static mp_obj_t papers3_sdcard_info(mp_obj_t self_in) {
    papers3_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->initialized) {
        mp_raise_ValueError(MP_ERROR_TEXT("SDCard not initialized"));
    }
    
    if (!self->mounted) {
        mp_raise_ValueError(MP_ERROR_TEXT("SDCard not mounted"));
    }
    
    // 导入os模块
    mp_obj_t os_module = mp_import_name(MP_QSTR_os, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
    
    // 调用 os.statvfs("/sd")
    mp_obj_t statvfs_args[1] = {
        mp_obj_new_str("/sd", 3)
    };
    
    mp_obj_t statvfs_method_args[3] = {os_module, MP_OBJ_NEW_QSTR(MP_QSTR_statvfs), statvfs_args[0]};
    mp_obj_t statvfs_result = mp_call_method_n_kw(1, 0, statvfs_method_args);
    
    // 解析statvfs结果: (bsize, frsize, blocks, bfree, bavail, files, ffree, favail, flag, namemax)
    mp_obj_t *statvfs_tuple;
    size_t tuple_len;
    mp_obj_get_array(statvfs_result, &tuple_len, &statvfs_tuple);
    
    if (tuple_len >= 5) {
        uint32_t bsize = mp_obj_get_int(statvfs_tuple[0]);    // 块大小
        uint32_t blocks = mp_obj_get_int(statvfs_tuple[2]);   // 总块数
        uint32_t bfree = mp_obj_get_int(statvfs_tuple[3]);    // 空闲块数
        
        uint64_t total_bytes = (uint64_t)bsize * blocks;
        uint64_t free_bytes = (uint64_t)bsize * bfree;
        uint64_t used_bytes = total_bytes - free_bytes;
        
        mp_obj_t info_tuple[4] = {
            mp_obj_new_int(total_bytes),  // 总容量
            mp_obj_new_int(used_bytes),   // 已用空间
            mp_obj_new_int(free_bytes),   // 可用空间
            statvfs_result                // 原始statvfs数据
        };
        
        return mp_obj_new_tuple(4, info_tuple);
    }
    
    return statvfs_result;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_sdcard_info_obj, papers3_sdcard_info);

// 方法表
static const mp_rom_map_elem_t papers3_sdcard_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_sdcard_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_sdcard_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&papers3_sdcard_mount_obj) },
    { MP_ROM_QSTR(MP_QSTR_unmount), MP_ROM_PTR(&papers3_sdcard_unmount_obj) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&papers3_sdcard_info_obj) },
};
static MP_DEFINE_CONST_DICT(papers3_sdcard_locals_dict, papers3_sdcard_locals_dict_table);

// 类型定义
MP_DEFINE_CONST_OBJ_TYPE(
    papers3_sdcard_type,
    MP_QSTR_SDCard,
    MP_TYPE_FLAG_NONE,
    make_new, papers3_sdcard_make_new,
    locals_dict, &papers3_sdcard_locals_dict
); 