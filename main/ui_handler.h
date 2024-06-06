#ifndef UI_HANDLER_H
#define UI_HANDLER_H
#include <stdio.h>

#include "lvgl.h"
#include "lv_conf.h"

void configure_UI(lv_disp_t *disp);

void send_input(uint16_t x, uint16_t y, bool state);

void show_xy(int16_t x, int16_t y, uint8_t s);

#endif