#ifndef UI_HANDLER_H
#define UI_HANDLER_H
#include <stdio.h>

#include "lvgl.h"
#include "lv_conf.h"

void configure_UI(lv_disp_t *disp);

void send_input(uint16_t x, uint16_t y, bool state);

 #endif