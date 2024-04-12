#ifndef LED_HANDLER_H
#define LED_HANDLER_H
#include <stdio.h>

void configure_led(void);
void add_point(int32_t loc, int32_t vel, int32_t sz, uint32_t col, uint32_t stren);
uint32_t hsv_to_col(double h, double s, double v);
 #endif