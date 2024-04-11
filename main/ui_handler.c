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

static const char *TAG = "GuitarOS(UI)";

static TickType_t last_input;

static lv_disp_t *pDisp = NULL;
static lv_obj_t * label;

static bool active = true;

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
    ESP_LOGI(TAG, "touch pressed = %d , x = %u , y=%u", state, x, y);
}