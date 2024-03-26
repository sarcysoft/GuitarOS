
/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

// This demo UI is adapted from LVGL official example: https://docs.lvgl.io/master/widgets/extra/meter.html#simple-meter

#include "lvgl.h"
#include "lv_conf.h"

static lv_obj_t *meter;
static lv_obj_t * btn;
static lv_disp_rot_t rotation = LV_DISP_ROT_NONE;

static void set_value(void *indic, int32_t v)
{
    lv_meter_set_indicator_end_value(meter, indic, v);
}

static void btn_cb(lv_event_t * e)
{
    lv_disp_t *disp = lv_event_get_user_data(e);
    rotation++;
    if (rotation > LV_DISP_ROT_270) {
        rotation = LV_DISP_ROT_NONE;
    }
    lv_disp_set_rotation(disp, rotation);
}

// BRG

void example_lvgl_demo_ui(lv_disp_t *disp)
{
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);

    /*Create a white label, set its text and align it to the center*/
    lv_obj_t * label = lv_label_create(scr);
    lv_label_set_text(label, "A#");
    lv_obj_set_style_text_color(label, lv_color_hex(0xFF0020), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * wheel = lv_colorwheel_create(scr, true);
    lv_obj_set_size(wheel, 240, 240);
    lv_colorwheel_set_mode_fixed(wheel, LV_COLORWHEEL_MODE_VALUE);
    lv_obj_center(wheel);
}
