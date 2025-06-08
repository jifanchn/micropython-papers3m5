#pragma once


#include <Arduino.h>
#include "ISprite.h"
#include "ST7789Parallel.h"

extern ST7789Parallel g_st7789;
extern ISprite g_sprite;

void graphics_init();
void enter_err_state(const char* msg, uint8_t is_restart);
