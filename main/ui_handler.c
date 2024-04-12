#include <string.h>
#include <stdio.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "guitar_cfg.h"
#include "ui_handler.h"
#include "lcd_handler.h"
#include "math.h"
#include "led_handler.h"

extern const lv_img_dsc_t ss_logo_240;

static const char *TAG = "GuitarOS(UI)";

static TickType_t last_input;
static int16_t last_zone_pressed = -1;
static int16_t last_zone_released = -1;
static int16_t last_zone = -1;
static bool zone_state = false;

static lv_disp_t *pDisp = NULL;

static lv_obj_t * logo = NULL;
static lv_obj_t * label = NULL;

#ifdef SHOW_TOUCH_ZONE
static lv_obj_t * touch_zone = NULL;
#endif
static bool active = true;

void disp_task( void * pvParameters )
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 25;

    lv_obj_t *scr = lv_disp_get_scr_act(pDisp);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);

    logo = lv_img_create(scr);
    lv_img_set_src(logo, &ss_logo_240);
    lv_obj_align(logo, LV_ALIGN_CENTER, 0, 0);

    /*Create a white label, set its text and align it to the center*/
/*
    label = lv_label_create(scr);
    lv_label_set_text(label, "A#");
    lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_PURPLE), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
*/
#ifdef SHOW_TOUCH_ZONE
    /*Create an Arc*/
    touch_zone = lv_arc_create(scr);
    lv_obj_set_size(touch_zone, 100, 100);
    lv_arc_set_rotation(touch_zone, 0);
    lv_arc_set_bg_angles(touch_zone, 0, 360);
    lv_obj_set_style_arc_width(touch_zone, 50, LV_PART_MAIN);
    lv_obj_set_style_arc_color(touch_zone, lv_palette_main(LV_PALETTE_PURPLE), LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(touch_zone, false, LV_PART_MAIN);
    lv_obj_remove_style(touch_zone, NULL, LV_PART_KNOB);
    lv_arc_set_value(touch_zone, 0);
    lv_obj_center(touch_zone);
#endif

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
    last_input = xTaskGetTickCount();

    for( ;; )
    {
        // Wait for the next cycle.
        vTaskDelayUntil( &xLastWakeTime, xFrequency );

        if (pdTICKS_TO_MS(xLastWakeTime - last_input) > 20000)
        {
            lcd_state(false);
            active = false;
        }
        else
        {
            if (!active)
            {
                lcd_state(true);
                active = true;
            }
        }

        // Perform action here.
    }
}

void configure_UI(lv_disp_t *disp)
{
    ESP_LOGI(TAG, "Configuring UI!"); 

    pDisp = disp;
    BaseType_t xReturned;
    TaskHandle_t xHandle = NULL;

    /* Create the task, storing the handle. */
    xReturned = xTaskCreate(
                    disp_task,       /* Function that implements the task. */
                    "Display Task",          /* Text name for the task. */
                    10240,      /* Stack size in words, not bytes. */
                    ( void * ) 1,    /* Parameter passed into the task. */
                    tskIDLE_PRIORITY,/* Priority at which the task is created. */
                    &xHandle );      /* Used to pass out the created task's handle. */

    if( xReturned == pdPASS )
    {
        /* The task was created.  Use the task's handle to delete the task. */
        //vTaskDelete( xHandle );
    }
}

void send_input(uint16_t x, uint16_t y, bool state)
{
    last_input = xTaskGetTickCount();

    double rel_x = x - 120;
    double rel_y = y - 120;
    uint16_t r = (uint16_t)sqrt((rel_x*rel_x)+(rel_y*rel_y));
    int16_t a = (360 + (int16_t)((180 * atan2(rel_y, rel_x) / 3.14159))) % 360;

    uint16_t rad = 50;
    uint16_t segs = 8;


    int16_t zone = 0;
    if (r < rad)
    {
#ifdef SHOW_TOUCH_ZONE
        lv_obj_set_size(touch_zone, (rad * 2), (rad * 2));
        lv_arc_set_bg_angles(touch_zone, 0, 360);
        lv_obj_set_style_arc_width(touch_zone, rad, LV_PART_MAIN);
#endif
    }
    else
    {
        uint16_t seg_ang = (360/segs);
        uint16_t segment = ((a + seg_ang/2) / seg_ang) % segs;
#ifdef SHOW_TOUCH_ZONE
        uint16_t ang = 540 - (segment * seg_ang);
        lv_obj_set_size(touch_zone, 240, 240);
        lv_arc_set_bg_angles(touch_zone, ang - (seg_ang/2), ang + (seg_ang/2));
        lv_obj_set_style_arc_width(touch_zone, 120 - rad, LV_PART_MAIN);
#endif
        zone = segment + 1;
    }

    if (state != zone_state)
    {    
        if (state)
        {
            ESP_LOGI(TAG, "Zone %d pressed", zone);
            last_zone_pressed = zone;

            add_point(0, 1+r/15, 5, hsv_to_col((double)a, 1, 1), 255);
        }
        else
        {
            ESP_LOGI(TAG, "Zone %d released", zone);
            last_zone_released = zone;
        }

        last_zone = zone;
        zone_state = state;
    }
    else if (zone != last_zone)
    {
        if (last_zone != -1)
        {
            ESP_LOGI(TAG, "Zone %d entered", zone);
            add_point(0, 1+r/15, 5, hsv_to_col((double)a, 1, 1), 255);
        }

        last_zone = zone;
    }


    //ESP_LOGI(TAG, "touch pressed = %d , x = %u , y=%u : vec = %u @ %d°", state, x, y, r, a);
    
}