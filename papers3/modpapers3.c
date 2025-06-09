/*
 * Papers3 MicroPython Module
 * Main module providing Papers3 board functionality
 */

#include "py/runtime.h"
#include "py/obj.h"
#include <string.h>
#include "esp_flash.h"
#include "esp_psram.h"
#include "esp_heap_caps.h"
#include "esp_chip_info.h"
#include "esp_system.h"
#include "soc/efuse_reg.h"

// Forward declarations for class constructors
extern const mp_obj_type_t papers3_buzzer_type;
extern const mp_obj_type_t papers3_battery_type;
extern const mp_obj_type_t papers3_gyro_type;
extern const mp_obj_type_t papers3_rtc_type;
extern const mp_obj_type_t papers3_epdiy_type;
extern const mp_obj_type_t papers3_led_type;
// 移除冲突模块: extern const mp_obj_type_t papers3_button_type;
extern const mp_obj_type_t papers3_touch_type;

// Info function
static mp_obj_t papers3_info(void) {
    mp_printf(&mp_plat_print, "Papers3 MicroPython Module\n");
    mp_printf(&mp_plat_print, "Board: ESP32S3-N16R8 (16MB Flash + 8MB PSRAM)\n");
    mp_printf(&mp_plat_print, "Available classes:\n");
    mp_printf(&mp_plat_print, "  - papers3.Buzzer() - PWM buzzer control\n");
    mp_printf(&mp_plat_print, "  - papers3.Battery() - Battery monitoring (real ADC)\n");
    mp_printf(&mp_plat_print, "  - papers3.Gyro() - BMI270 accelerometer + gyroscope\n");
    mp_printf(&mp_plat_print, "  - papers3.RTC() - BM8563 real-time clock\n");
    mp_printf(&mp_plat_print, "  - papers3.EPDiy() - E-Paper display control with Chinese font support\n");
    mp_printf(&mp_plat_print, "  - papers3.LED() - LED control (GPIO 0)\n");
    // 移除冲突模块: mp_printf(&mp_plat_print, "  - papers3.Button() - Button and status detection (GPIO 4, 5)\n");
    mp_printf(&mp_plat_print, "  - papers3.Touch() - GT911 capacitive touch screen\n");
    mp_printf(&mp_plat_print, "System functions:\n");
    mp_printf(&mp_plat_print, "  - papers3.flash_info() - Flash memory information\n");
    mp_printf(&mp_plat_print, "  - papers3.ram_info() - RAM memory information\n");
    mp_printf(&mp_plat_print, "Demo programs available in /demo directory\n");

    mp_printf(&mp_plat_print, "Note: Using ESP-IDF I2C drivers for hardware compatibility\n");
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(papers3_info_obj, papers3_info);

// Flash information
static mp_obj_t papers3_flash_info(void) {
    mp_obj_t dict = mp_obj_new_dict(0);
    
    // Get flash chip information
    uint32_t flash_size = 0;
    esp_err_t err = esp_flash_get_size(NULL, &flash_size);
    if (err == ESP_OK) {
        mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_total_size), mp_obj_new_int(flash_size));
        mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_total_mb), mp_obj_new_int(flash_size / (1024 * 1024)));
    }
    
    // Get chip info for flash frequency
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_chip_model), mp_obj_new_str("ESP32S3", 7));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_chip_revision), mp_obj_new_int(chip_info.revision));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_cpu_cores), mp_obj_new_int(chip_info.cores));
    
    // Flash features
    const char* features = "";
    if (chip_info.features & CHIP_FEATURE_WIFI_BGN) features = "WiFi ";
    if (chip_info.features & CHIP_FEATURE_BLE) features = "WiFi+BLE ";
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_features), mp_obj_new_str(features, strlen(features)));
    
    mp_printf(&mp_plat_print, "Flash: %d MB total\n", flash_size / (1024 * 1024));
    
    return dict;
}
static MP_DEFINE_CONST_FUN_OBJ_0(papers3_flash_info_obj, papers3_flash_info);

// RAM information  
static mp_obj_t papers3_ram_info(void) {
    mp_obj_t dict = mp_obj_new_dict(0);
    
    // Internal RAM
    size_t internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t internal_used = internal_total - internal_free;
    size_t internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_internal_total), mp_obj_new_int(internal_total));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_internal_free), mp_obj_new_int(internal_free));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_internal_used), mp_obj_new_int(internal_used));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_internal_largest), mp_obj_new_int(internal_largest));
    
    // PSRAM (if available)
    if (esp_psram_is_initialized()) {
        size_t psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t psram_used = psram_total - psram_free;
        size_t psram_largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
        
        mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_psram_total), mp_obj_new_int(psram_total));
        mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_psram_free), mp_obj_new_int(psram_free));
        mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_psram_used), mp_obj_new_int(psram_used));
        mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_psram_largest), mp_obj_new_int(psram_largest));
        
        mp_printf(&mp_plat_print, "RAM: Internal %d KB, PSRAM %d KB\n", 
                  internal_total / 1024, psram_total / 1024);
    } else {
        mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_psram_total), mp_obj_new_int(0));
        mp_printf(&mp_plat_print, "RAM: Internal %d KB, No PSRAM\n", internal_total / 1024);
    }
    
    return dict;
}
static MP_DEFINE_CONST_FUN_OBJ_0(papers3_ram_info_obj, papers3_ram_info);



// Module globals table
static const mp_rom_map_elem_t papers3_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_papers3) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&papers3_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_info), MP_ROM_PTR(&papers3_flash_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_ram_info), MP_ROM_PTR(&papers3_ram_info_obj) },

    
    // Object-oriented classes only
    { MP_ROM_QSTR(MP_QSTR_Buzzer), MP_ROM_PTR(&papers3_buzzer_type) },
    { MP_ROM_QSTR(MP_QSTR_Battery), MP_ROM_PTR(&papers3_battery_type) },
    { MP_ROM_QSTR(MP_QSTR_Gyro), MP_ROM_PTR(&papers3_gyro_type) },
    { MP_ROM_QSTR(MP_QSTR_RTC), MP_ROM_PTR(&papers3_rtc_type) },
    { MP_ROM_QSTR(MP_QSTR_EPDiy), MP_ROM_PTR(&papers3_epdiy_type) },
    { MP_ROM_QSTR(MP_QSTR_LED), MP_ROM_PTR(&papers3_led_type) },
    // 移除冲突模块: { MP_ROM_QSTR(MP_QSTR_Button), MP_ROM_PTR(&papers3_button_type) },
    { MP_ROM_QSTR(MP_QSTR_Touch), MP_ROM_PTR(&papers3_touch_type) },
};

static MP_DEFINE_CONST_DICT(papers3_globals, papers3_globals_table);

// Define the module
const mp_obj_module_t mp_module_papers3 = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&papers3_globals,
};

// Register the module to make it available from Python
MP_REGISTER_MODULE(MP_QSTR_papers3, mp_module_papers3); 