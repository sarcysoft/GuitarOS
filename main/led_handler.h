#ifndef LED_HANDLER_H
#define LED_HANDLER_H
#include <stdio.h>

typedef struct rgb_type {
    uint32_t r;
    uint32_t g;
    uint32_t b;
    uint32_t s;
} rgb_t;

void configure_led(void);
void add_point(int32_t loc, int32_t vel, int32_t sz, uint32_t col, uint32_t stren);
void set_col(uint32_t col);
rgb_t waveLengthToRGB(double Wavelength, int8_t stren);
uint32_t waveLengthToCol(double Wavelength, int8_t stren);

uint32_t rgb_to_col(double r, double g, double b);
uint32_t hsv_to_col(double h, double s, double v);
 #endif