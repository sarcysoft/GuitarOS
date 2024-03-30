#include "lvgl.h"
#include "lv_conf.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

static lv_obj_t *meter;
static lv_obj_t * btn;
static lv_disp_rot_t rotation = LV_DISP_ROT_NONE;
static lv_disp_t *pDisp = NULL;

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

lv_obj_t * label;
lv_obj_t * wheel;

static void wheel_cb(lv_event_t * event)
{
    lv_color_t col = lv_colorwheel_get_rgb(wheel);
    lv_obj_set_style_text_color(label, col, LV_PART_MAIN);
}

void disp_task( void * pvParameters )
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 25;

    lv_obj_t *scr = lv_disp_get_scr_act(pDisp);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);

    /*Create a white label, set its text and align it to the center*/
    label = lv_label_create(scr);
    lv_label_set_text(label, "A#");
    lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_PURPLE), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    wheel = lv_colorwheel_create(scr, true);
    lv_obj_add_event_cb(wheel, wheel_cb, LV_EVENT_VALUE_CHANGED , NULL);
    lv_obj_set_size(wheel, 220, 220);
    lv_colorwheel_set_mode_fixed(wheel, LV_COLORWHEEL_MODE_HUE);
    lv_obj_center(wheel);

     // Initialise the xLastWakeTime variable with the current time.
     xLastWakeTime = xTaskGetTickCount();

     for( ;; )
     {
         // Wait for the next cycle.
         vTaskDelayUntil( &xLastWakeTime, xFrequency );

         // Perform action here.
     }
}

void example_lvgl_demo_ui(lv_disp_t *disp)
{
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
