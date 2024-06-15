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

//#define SHOW_TOUCH_ZONE

extern const lv_img_dsc_t ss_logo_240;
extern const lv_img_dsc_t ss_logo_240_bg;

static const char *TAG = "GuitarOS(UI)";

static TickType_t last_input;
static TickType_t last_press;
static TickType_t last_hold;
static int16_t hold_time = 500;
static int16_t last_zone_pressed = -1;
static int16_t last_zone_released = -1;
static int16_t last_zone = -1;
static bool zone_state = false;

lv_obj_t * lblTitle;
lv_obj_t * lblDetails;

static lv_disp_t *pDisp = NULL;

static bool active = true;
static int32_t value = 0;

typedef enum
{
    eTouch,
    eEnter,
    eHold,
    ePress,
    eLongPress,
    eExtraLongPress
} event_t;


extern bool example_lvgl_lock(int timeout_ms);
extern void example_lvgl_unlock(void);

void handle_event(event_t event, uint8_t zone)
{
    if (!active)
    {
        if ((event == eExtraLongPress) && (zone == 0))
        {
            if (example_lvgl_lock(-1))
            {
                active = true;
                lcd_state(true);               
                example_lvgl_unlock();
            }
        }
    }
    else
    {
        switch (event)
        {
            case eTouch:
                ESP_LOGI(TAG, "Zone %d touched", zone);
            break;

            case eEnter:
                ESP_LOGI(TAG, "Zone %d entered", zone);
            break;
            
            case ePress:
                ESP_LOGI(TAG, "Press event");
                if (zone == 4)
                {
                    value++;
                    char text[16];
                    sprintf(text, "%d", (int)value);
                    lv_label_set_text(lblDetails, text);

                }
                else if (zone == 2)
                {
                    value--;
                    char text[16];
                    sprintf(text, "%d", (int)value);
                    lv_label_set_text(lblDetails, text);
                }
            break;

            case eHold:
                //ESP_LOGI(TAG, "Hold event");
                if (zone == 4)
                {
                    value++;
                    char text[16];
                    sprintf(text, "%d", (int)value);
                    lv_label_set_text(lblDetails, text);

                }
                else if (zone == 2)
                {
                    value--;
                    char text[16];
                    sprintf(text, "%d", (int)value);
                    lv_label_set_text(lblDetails, text);
                }
            break;

            case eLongPress:
                ESP_LOGI(TAG, "Long press event");
            break;

            case eExtraLongPress:
                ESP_LOGI(TAG, "Extra long press event");
                if (zone == 0)
                {
                    if (example_lvgl_lock(-1))
                    {
                        active = false;
                        lcd_state(false);               
                        example_lvgl_unlock();
                    }
                }
            break;

            default:
                ESP_LOGI(TAG, "Invalid event");
            break;
        }
    }
}

void disp_task( void * pvParameters )
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 10;

    if (example_lvgl_lock(-1))
    {
        lv_obj_t *scr = lv_disp_get_scr_act(pDisp);
        lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);

        lv_obj_t * bg = lv_img_create(scr);
        lv_img_set_src(bg, &ss_logo_240_bg);
        lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);

        lblTitle = lv_label_create(scr);
        lv_obj_set_style_text_font(lblTitle, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(lblTitle, lv_color_hex(0xff0000), 0);
        lv_label_set_text(lblTitle, "Threshold");
        lv_obj_set_width(lblTitle, 150);  
        lv_obj_set_style_text_align(lblTitle, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lblTitle, LV_ALIGN_CENTER, 0, -40);

        lblDetails = lv_label_create(scr);
        lv_obj_set_style_text_font(lblDetails, &lv_font_montserrat_38, 0);
        lv_obj_set_style_text_color(lblDetails, lv_color_hex(0x00ff00), 0);
        lv_label_set_text(lblDetails, "0");
        lv_obj_set_width(lblDetails, 150); 
        lv_obj_set_style_text_align(lblDetails, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lblDetails, LV_ALIGN_CENTER, 0, 0);
        
        lv_obj_t * logo = lv_img_create(scr);
        lv_img_set_src(logo, &ss_logo_240);
        lv_obj_align(logo, LV_ALIGN_CENTER, 0, 0);

        lv_obj_del_delayed(logo,3000);

        example_lvgl_unlock();
    }

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
    last_input = xTaskGetTickCount();
    last_press = xTaskGetTickCount();

    for( ;; )
    {
        // Wait for the next cycle.
        vTaskDelayUntil( &xLastWakeTime, xFrequency );

        if (zone_state)
        {
            if (pdTICKS_TO_MS(xTaskGetTickCount() - last_hold) > hold_time)
            {
                handle_event(eHold, last_zone);
                last_hold = xTaskGetTickCount();
                if (hold_time > 10)
                {
                    hold_time = hold_time / 2;
                }
            }
        }
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


uint16_t rad = 50;
uint16_t segs = 4;

void show_touch_zone(int8_t zone)
{
#ifdef SHOW_TOUCH_ZONE
    static lv_obj_t * touch_zone = NULL;

    if (example_lvgl_lock(10))
    {
        if (zone >= 0)
        {
            if (touch_zone == NULL)
            {
                lv_obj_t *scr = lv_disp_get_scr_act(pDisp);

                touch_zone = lv_arc_create(scr);
                lv_arc_set_rotation(touch_zone, 0);
                lv_obj_set_style_arc_color(touch_zone, lv_palette_main(LV_PALETTE_PURPLE), LV_PART_MAIN);
                lv_obj_set_style_arc_rounded(touch_zone, false, LV_PART_MAIN);
                lv_obj_remove_style(touch_zone, NULL, LV_PART_KNOB);
                lv_arc_set_value(touch_zone, 0);
                lv_obj_center(touch_zone);
            }

            if (zone == 0)
            {
                lv_obj_set_size(touch_zone, (rad * 2), (rad * 2));
                lv_arc_set_bg_angles(touch_zone, 0, 360);
                lv_obj_set_style_arc_width(touch_zone, rad, LV_PART_MAIN);
            }
            else
            {
                zone--;
                uint16_t seg_ang = (360/segs);
                uint16_t ang = (zone * seg_ang) + 360;

                lv_obj_set_size(touch_zone, 240, 240);
                lv_arc_set_bg_angles(touch_zone, ang - (seg_ang/2), ang + (seg_ang/2));
                lv_obj_set_style_arc_width(touch_zone, 120 - rad, LV_PART_MAIN);
            }
        }
        else
        {
            if (touch_zone != NULL)
            {
                lv_obj_del(touch_zone);
                touch_zone = NULL;
            }
        }

        example_lvgl_unlock();
    }
#endif
}


void send_input(uint16_t x, uint16_t y, bool state)
{
    last_input = xTaskGetTickCount();

    double rel_x = x - 120;
    double rel_y = y - 120;
    uint16_t r = (uint16_t)sqrt((rel_x*rel_x)+(rel_y*rel_y));
    int16_t a = (360 + (int16_t)((180 * atan2(rel_y, rel_x) / 3.14159))) % 360;


    int16_t zone = 0;
    if (r < rad)
    {

    }
    else
    {
        uint16_t seg_ang = (360/segs);
        uint16_t segment = ((a + seg_ang/2) / seg_ang) % segs;
        zone = segment + 1;
    }

    if (state != zone_state)
    {    
        if (state)
        {
            last_zone_pressed = zone;
            last_hold = last_press = xTaskGetTickCount();
            hold_time = 500;
            show_touch_zone(zone);
            handle_event(eTouch, zone);
        }
        else
        {
            show_touch_zone(-1);
            last_zone_released = zone;
            if (pdTICKS_TO_MS(xTaskGetTickCount() - last_press) > 2000)
            {
                handle_event(eExtraLongPress, zone);
            }
            else if (pdTICKS_TO_MS(xTaskGetTickCount() - last_press) > 500)
            {
                handle_event(eLongPress, zone);
            }
            else
            {
                handle_event(ePress, zone);
            }
        }

        last_zone = zone;
        zone_state = state;
    }
    else if (zone != last_zone)
    {
        if (last_zone != -1)
        {
            show_touch_zone(zone);
            handle_event(eEnter, zone);
            last_hold = xTaskGetTickCount();
        }

        last_zone = zone;
    }

    //ESP_LOGI(TAG, "touch pressed = %d , x = %u , y=%u : vec = %u @ %d°", state, x, y, r, a);
}