/*
 * Papers3 EPDiy Sub-module
 * E-Paper Display control for Papers3 board
 * Based on successful papers3-esp-demo implementation
 */

#include "py/runtime.h"
#include "py/obj.h"
#include <string.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

// Include EPDiy headers (highlevel API like original demo)
#include "epdiy.h"
#include "epd_highlevel.h"
#include "epd_display.h"
#include "epd_board.h"

// static const char* TAG = "papers3_epdiy";  // Unused variable

// Display configuration
#define DISPLAY_WIDTH 960
#define DISPLAY_HEIGHT 540
#define FRAMEBUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 2)  // 4 bits per pixel

// Global state
static uint8_t* framebuffer = NULL;
static bool epdiy_initialized = false;
static bool epdiy_board_initialized = false;
static EpdiyHighlevelState hl_state;

// External board definition
extern const EpdBoardDefinition epd_board_papers3;

// Helper function to validate color values (0x00-0xF0 in 0x10 steps)
static bool validate_color(uint8_t color) {
    return (color <= 0xF0) && ((color & 0x0F) == 0);
}

// Helper function to set pixel in framebuffer
static void set_pixel_in_buffer(int x, int y, uint8_t color) {
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT || !framebuffer) {
        return;
    }
    
    int index = (y * DISPLAY_WIDTH + x) / 2;
    uint8_t nibble = color >> 4;  // Convert 0x00-0xF0 to 0x0-0xF
    
    if (x % 2 == 0) {
        // Even x: use lower nibble
        framebuffer[index] = (framebuffer[index] & 0x0F) | (nibble << 4);
    } else {
        // Odd x: use upper nibble  
        framebuffer[index] = (framebuffer[index] & 0xF0) | nibble;
    }
}

// Initialize EPD (like original demo)
static mp_obj_t papers3_epdiy_init(void) {
    if (epdiy_initialized) {
        mp_printf(&mp_plat_print, "EPD already initialized\n");
        return mp_const_true;
    }
    
    mp_printf(&mp_plat_print, "Initializing Papers3 EPD...\n");
    
    // Initialize EPDiy exactly like original demo
    epd_init(&epd_board_papers3, &ED047TC2, EPD_LUT_64K);
    hl_state = epd_hl_init(EPD_BUILTIN_WAVEFORM);
    framebuffer = epd_hl_get_framebuffer(&hl_state);
    
    if (framebuffer) {
        epdiy_board_initialized = true;
        epdiy_initialized = true;
        mp_printf(&mp_plat_print, "EPDiy hardware integration: Active\n");
        mp_printf(&mp_plat_print, "Framebuffer allocated: %d bytes\n", FRAMEBUFFER_SIZE);
        mp_printf(&mp_plat_print, "Papers3 EPD initialization complete\n");
    } else {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to initialize EPDiy highlevel"));
    }
    
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_0(papers3_epdiy_init_obj, papers3_epdiy_init);

// Deinitialize EPD  
static mp_obj_t papers3_epdiy_deinit(void) {
    if (!epdiy_initialized) {
        return mp_const_false;
    }
    
    if (epdiy_board_initialized) {
        epd_deinit();
        epdiy_board_initialized = false;
    }
    
    framebuffer = NULL;
    epdiy_initialized = false;
    mp_printf(&mp_plat_print, "Papers3 EPD deinitialized\n");
    
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_0(papers3_epdiy_deinit_obj, papers3_epdiy_deinit);

// Clear display
static mp_obj_t papers3_epdiy_clear(void) {
    if (!framebuffer) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPD not initialized"));
    }
    
    if (epdiy_board_initialized) {
        epd_hl_set_all_white(&hl_state);
        mp_printf(&mp_plat_print, "Display buffer cleared (hardware)\n");
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(papers3_epdiy_clear_obj, papers3_epdiy_clear);

// Update display (like original demo)
static mp_obj_t papers3_epdiy_update(void) {
    if (!framebuffer) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPD not initialized"));
    }
    
    if (epdiy_board_initialized) {
        mp_printf(&mp_plat_print, "Updating EPD display (temperature: 25°C)...\n");
        
        epd_poweron();
        enum EpdDrawError result = epd_hl_update_screen(&hl_state, MODE_GC16, 25);
        epd_poweroff();
        
        if (result == EPD_DRAW_SUCCESS) {
            mp_printf(&mp_plat_print, "EPD update completed successfully\n");
        } else {
            mp_printf(&mp_plat_print, "EPD update failed with error: 0x%x\n", result);
        }
    } else {
        mp_printf(&mp_plat_print, "EPD update (simulated - no hardware)\n");
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(papers3_epdiy_update_obj, papers3_epdiy_update);

// Draw pixel
static mp_obj_t papers3_epdiy_draw_pixel(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t color_obj) {
    if (!framebuffer) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPD not initialized"));
    }
    
    int x = mp_obj_get_int(x_obj);
    int y = mp_obj_get_int(y_obj);
    uint8_t color = mp_obj_get_int(color_obj) & 0xF0;
    
    if (!validate_color(color)) {
        mp_raise_ValueError(MP_ERROR_TEXT("color must be 0x00-0xF0 in 0x10 steps"));
    }
    
    // Use EPDiy's pixel function
    epd_draw_pixel(x, y, color, framebuffer);
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(papers3_epdiy_draw_pixel_obj, papers3_epdiy_draw_pixel);

// Draw text
static mp_obj_t papers3_epdiy_draw_text(size_t n_args, const mp_obj_t *args) {
    if (!framebuffer) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("EPD not initialized"));
    }
    
    int x = mp_obj_get_int(args[0]);
    int y = mp_obj_get_int(args[1]);
    const char* text = mp_obj_str_get_str(args[2]);
    uint8_t color = mp_obj_get_int(args[3]) & 0xF0;
    
    if (!validate_color(color)) {
        mp_raise_ValueError(MP_ERROR_TEXT("color must be 0x00-0xF0 in 0x10 steps"));
    }
    
    // Simple text rendering - draw character placeholders
    int char_width = 8, char_height = 12;
    for (int i = 0; text[i] != '\0'; i++) {
        int char_x = x + i * (char_width + 2);
        
        // Draw simple rectangle for each character
        for (int dy = 0; dy < char_height; dy++) {
            for (int dx = 0; dx < char_width; dx++) {
                if (dx == 0 || dy == 0 || dx == char_width-1 || dy == char_height-1) {
                    epd_draw_pixel(char_x + dx, y + dy, color, framebuffer);
                }
            }
        }
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(papers3_epdiy_draw_text_obj, 4, 4, papers3_epdiy_draw_text);

// Get stats
static mp_obj_t papers3_epdiy_stats(void) {
    mp_obj_t dict = mp_obj_new_dict(0);
    
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_initialized), mp_obj_new_bool(epdiy_initialized));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_framebuffer), mp_obj_new_bool(framebuffer != NULL));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_buffer_size), mp_obj_new_int(FRAMEBUFFER_SIZE));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_width), mp_obj_new_int(DISPLAY_WIDTH));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_height), mp_obj_new_int(DISPLAY_HEIGHT));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_epdiy_hw), mp_obj_new_bool(epdiy_board_initialized));
    
    return dict;
}
static MP_DEFINE_CONST_FUN_OBJ_0(papers3_epdiy_stats_obj, papers3_epdiy_stats);

// EPDiy sub-module globals table
static const mp_rom_map_elem_t papers3_epdiy_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_papers3_epdiy) },
    
    // Core functions
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&papers3_epdiy_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&papers3_epdiy_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&papers3_epdiy_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&papers3_epdiy_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_stats), MP_ROM_PTR(&papers3_epdiy_stats_obj) },
    
    // Drawing functions
    { MP_ROM_QSTR(MP_QSTR_draw_pixel), MP_ROM_PTR(&papers3_epdiy_draw_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_text), MP_ROM_PTR(&papers3_epdiy_draw_text_obj) },
};
static MP_DEFINE_CONST_DICT(papers3_epdiy_module_globals, papers3_epdiy_module_globals_table);

// Define the EPDiy sub-module
const mp_obj_module_t papers3_epdiy_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&papers3_epdiy_module_globals,
}; 