/*
 * Papers3 SD Card Module
 * Updated to use correct pins from papers3-esp-demo
 */

#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "esp_log.h"
#include "papers3_pins.h"

#define TAG "papers3_sdcard"

// SD Card SPI Configuration (from Papers3 Demo)
// SCK=39, MISO=40, MOSI=38, CS=47
#define SD_SCK_PIN      PAPERS3_SD_SCK
#define SD_MISO_PIN     PAPERS3_SD_MISO
#define SD_MOSI_PIN     PAPERS3_SD_MOSI
#define SD_CS_PIN       PAPERS3_SD_CS
#define SD_FREQ         20000000 // 20MHz

typedef struct _papers3_sdcard_obj_t {
    mp_obj_base_t base;
    bool initialized;
    bool mounted;
    mp_obj_t spi_obj;
    mp_obj_t cs_pin_obj;
    mp_obj_t vfs_obj;
} papers3_sdcard_obj_t;

static mp_obj_t papers3_sdcard_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    extern const mp_obj_type_t papers3_sdcard_type;
    papers3_sdcard_obj_t *self = mp_obj_malloc(papers3_sdcard_obj_t, &papers3_sdcard_type);
    self->initialized = false;
    self->mounted = false;
    self->spi_obj = mp_const_none;
    self->cs_pin_obj = mp_const_none;
    self->vfs_obj = mp_const_none;
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t papers3_sdcard_init(mp_obj_t self_in) {
    papers3_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    mp_obj_t machine = mp_import_name(MP_QSTR_machine, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
    mp_obj_t Pin = mp_load_attr(machine, MP_QSTR_Pin);
    mp_obj_t SPI = mp_load_attr(machine, MP_QSTR_SPI);
    
    // CS Pin
    mp_obj_t pin_args[] = { mp_obj_new_int(SD_CS_PIN), mp_load_attr(Pin, MP_QSTR_OUT) };
    self->cs_pin_obj = mp_call_function_n_kw(Pin, 2, 0, pin_args);
    mp_obj_t pin_high[] = { mp_obj_new_int(1) };
    mp_call_method_n_kw(1, 0, (mp_obj_t[]){ self->cs_pin_obj, MP_OBJ_NEW_QSTR(MP_QSTR_value), pin_high[0] });
    
    // SPI
    // SPI(1, baudrate=20MHz, polarity=0, phase=0, sck=39, mosi=38, miso=40)
    mp_obj_t spi_args[] = { mp_obj_new_int(1) };
    mp_obj_t spi_kw_vals[] = {
        mp_obj_new_int(SD_FREQ), mp_obj_new_int(0), mp_obj_new_int(0),
        mp_call_function_1(Pin, mp_obj_new_int(SD_SCK_PIN)),
        mp_call_function_1(Pin, mp_obj_new_int(SD_MOSI_PIN)),
        mp_call_function_1(Pin, mp_obj_new_int(SD_MISO_PIN))
    };
    
    mp_map_elem_t spi_kw_map[] = {
        { MP_OBJ_NEW_QSTR(MP_QSTR_baudrate), spi_kw_vals[0] },
        { MP_OBJ_NEW_QSTR(MP_QSTR_polarity), spi_kw_vals[1] },
        { MP_OBJ_NEW_QSTR(MP_QSTR_phase),    spi_kw_vals[2] },
        { MP_OBJ_NEW_QSTR(MP_QSTR_sck),      spi_kw_vals[3] },
        { MP_OBJ_NEW_QSTR(MP_QSTR_mosi),     spi_kw_vals[4] },
        { MP_OBJ_NEW_QSTR(MP_QSTR_miso),     spi_kw_vals[5] },
    };
    
    self->spi_obj = mp_call_function_n_kw(SPI, 1, 6, (mp_obj_t[]){ 
        spi_args[0],
        MP_OBJ_NEW_QSTR(MP_QSTR_baudrate), spi_kw_vals[0],
        MP_OBJ_NEW_QSTR(MP_QSTR_polarity), spi_kw_vals[1],
        MP_OBJ_NEW_QSTR(MP_QSTR_phase),    spi_kw_vals[2],
        MP_OBJ_NEW_QSTR(MP_QSTR_sck),      spi_kw_vals[3],
        MP_OBJ_NEW_QSTR(MP_QSTR_mosi),     spi_kw_vals[4],
        MP_OBJ_NEW_QSTR(MP_QSTR_miso),     spi_kw_vals[5]
    });
    
    // SDCard VFS
    mp_obj_t sdcard_cls = mp_load_attr(machine, MP_QSTR_SDCard);
    
    // SDCard(slot=2, sck=..., mosi=..., miso=..., cs=...) -> Slot 2 for SPI? Slot 1 is SDMMC.
    // Usually SPI SDCard in MicroPython doesn't use 'slot' arg in latest versions, 
    // it uses 'spi' and 'cs' args.
    // Let's check typical MicroPython SDCard constructor:
    // class machine.SDCard(slot=1, width=1, cd=None, wp=None, sck=None, miso=None, mosi=None, cs=None, freq=20000000)
    
    mp_obj_t sd_kw_vals[] = {
        spi_kw_vals[3], spi_kw_vals[4], spi_kw_vals[5], self->cs_pin_obj
    };
    
    self->vfs_obj = mp_call_function_n_kw(sdcard_cls, 1, 4, (mp_obj_t[]){
        mp_obj_new_int(1), // Slot 1? Or 2? 
        MP_OBJ_NEW_QSTR(MP_QSTR_sck),  sd_kw_vals[0],
        MP_OBJ_NEW_QSTR(MP_QSTR_mosi), sd_kw_vals[1],
        MP_OBJ_NEW_QSTR(MP_QSTR_miso), sd_kw_vals[2],
        MP_OBJ_NEW_QSTR(MP_QSTR_cs),   sd_kw_vals[3]
    });
    
    self->initialized = true;
    ESP_LOGI(TAG, "SDCard Initialized");
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_sdcard_init_obj, papers3_sdcard_init);

static mp_obj_t papers3_sdcard_deinit(mp_obj_t self_in) {
    papers3_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->initialized) {
        if (self->mounted) {
            mp_obj_t os = mp_import_name(MP_QSTR_os, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
            mp_call_method_n_kw(1, 0, (mp_obj_t[]){ os, MP_OBJ_NEW_QSTR(MP_QSTR_umount), mp_obj_new_str("/sd", 3) });
            self->mounted = false;
        }
        self->initialized = false;
        self->spi_obj = mp_const_none;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_sdcard_deinit_obj, papers3_sdcard_deinit);

static mp_obj_t papers3_sdcard_mount(size_t n_args, const mp_obj_t *args) {
    papers3_sdcard_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->initialized) mp_raise_ValueError(MP_ERROR_TEXT("Not initialized"));
    if (self->mounted) mp_raise_ValueError(MP_ERROR_TEXT("Already mounted"));
    
    const char *path = "/sd";
    if (n_args > 1) path = mp_obj_str_get_str(args[1]);
    
    mp_obj_t os = mp_import_name(MP_QSTR_os, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
    mp_call_method_n_kw(2, 0, (mp_obj_t[]){ os, MP_OBJ_NEW_QSTR(MP_QSTR_mount), self->vfs_obj, mp_obj_new_str(path, strlen(path)) });
    
    self->mounted = true;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_sdcard_mount_obj, 1, 2, papers3_sdcard_mount);

static mp_obj_t papers3_sdcard_unmount(mp_obj_t self_in) {
    papers3_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->mounted) return mp_const_none;
    
    mp_obj_t os = mp_import_name(MP_QSTR_os, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
    mp_call_method_n_kw(1, 0, (mp_obj_t[]){ os, MP_OBJ_NEW_QSTR(MP_QSTR_umount), mp_obj_new_str("/sd", 3) });
    
    self->mounted = false;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_sdcard_unmount_obj, papers3_sdcard_unmount);

static mp_obj_t papers3_sdcard_info(mp_obj_t self_in) {
    papers3_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->mounted) mp_raise_ValueError(MP_ERROR_TEXT("Not mounted"));
    
    mp_obj_t os = mp_import_name(MP_QSTR_os, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
    mp_obj_t res = mp_call_method_n_kw(1, 0, (mp_obj_t[]){ os, MP_OBJ_NEW_QSTR(MP_QSTR_statvfs), mp_obj_new_str("/sd", 3) });
    return res; 
}
static MP_DEFINE_CONST_FUN_OBJ_1(papers3_sdcard_info_obj, papers3_sdcard_info);

static const mp_rom_map_elem_t papers3_sdcard_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_sdcard_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_sdcard_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&papers3_sdcard_mount_obj) },
    { MP_ROM_QSTR(MP_QSTR_unmount), MP_ROM_PTR(&papers3_sdcard_unmount_obj) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&papers3_sdcard_info_obj) },
};
static MP_DEFINE_CONST_DICT(papers3_sdcard_locals_dict, papers3_sdcard_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    papers3_sdcard_type,
    MP_QSTR_SDCard,
    MP_TYPE_FLAG_NONE,
    make_new, papers3_sdcard_make_new,
    locals_dict, &papers3_sdcard_locals_dict
);