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
#include "touch_handler.h"
#include "ui_handler.h"
#include "esp_lcd_touch_cst816s.h"
#include "driver/i2c.h"

#include "lvgl.h"
#include "lv_conf.h"

static const char *TAG = "GuitarOS(Touch)";
static SemaphoreHandle_t touch_mux;
static esp_lcd_touch_handle_t tp;
static lv_indev_t * indev_touchpad;
static lv_indev_data_t data;

#define CONFIG_LCD_HRES 240
#define CONFIG_LCD_VRES 240
#define CONFIG_LCD_TOUCH_RST 13
#define CONFIG_LCD_TOUCH_INT 5

#define MEMS_ADDR 0x6B

static void touch_callback(esp_lcd_touch_handle_t tp)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(touch_mux, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    esp_lcd_touch_read_data(tp); // read only when ISR was triggled
    uint16_t touch_x[1];
    uint16_t touch_y[1];
    uint16_t touch_strength[1];
    uint8_t touch_cnt = 0;

    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, touch_x, touch_y, touch_strength, &touch_cnt, 1);

    if ((touch_x[0] < 240) && (touch_y[0] < 240))
    {
        data->point.x = 240-touch_x[0];
        data->point.y = touch_y[0];
        if (touchpad_pressed)
        {
            data->state = LV_INDEV_STATE_PR;
        }
        else
        {
            data->state = LV_INDEV_STATE_REL;
        }
    }    
}

void touch_task( void * pvParameters )
{
    static uint16_t last_x = 0xffff;
    static uint16_t last_y = 0xffff;
    static bool last_pressed = false;
    
    while (true)
    {
        xSemaphoreTake(touch_mux, portMAX_DELAY);

        esp_lcd_touch_read_data(tp); // read only when ISR was triggled
        uint16_t touch_x[1];
        uint16_t touch_y[1];
        uint16_t touch_strength[1];
        uint8_t touch_cnt = 0;

        bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, touch_x, touch_y, touch_strength, &touch_cnt, 1);

        if ((touch_x[0] < 240) && (touch_y[0] < 240))
        {
            if ((last_x != touch_x[0]) || (last_y != touch_y[0]) || (last_pressed != touchpad_pressed))
            {
                last_x = touch_x[0];
                last_y = touch_y[0];
                last_pressed = touchpad_pressed;
                send_input(touch_x[0], touch_y[0], last_pressed);

                lv_indev_data_t data;
                data.point.x = last_x;
                data.point.y = last_y;
                if (last_pressed)
                {
                    data.state = LV_INDEV_STATE_PR;
                }
                else
                {
                    data.state = LV_INDEV_STATE_REL;
                }
            }
        }
    }
}

void i2c_init(void)
{
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 6,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = 7,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000
    };
    i2c_param_config(0, &i2c_conf);
    i2c_driver_install(0, i2c_conf.mode, 0, 0, 0);
    
}

void configure_touch(void)
{
    ESP_LOGI(TAG, "Configuring Touch!");

    i2c_init();

    touch_mux = xSemaphoreCreateBinary();

    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = CONFIG_LCD_HRES,
        .y_max = CONFIG_LCD_VRES,
        .rst_gpio_num = CONFIG_LCD_TOUCH_RST,
        .int_gpio_num = CONFIG_LCD_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .interrupt_callback = touch_callback,
    };

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)0 , &io_config, &io_handle);
    esp_lcd_touch_new_i2c_cst816s(io_handle, &tp_cfg, &tp);

#if 1
    BaseType_t xReturned;
    TaskHandle_t xHandle = NULL;

    /* Create the task, storing the handle. */
    xReturned = xTaskCreate(
                    touch_task,       /* Function that implements the task. */
                    "Touch Task",          /* Text name for the task. */
                    10240,      /* Stack size in words, not bytes. */
                    ( void * ) 1,    /* Parameter passed into the task. */
                    tskIDLE_PRIORITY,/* Priority at which the task is created. */
                    &xHandle );      /* Used to pass out the created task's handle. */

    if( xReturned == pdPASS )
    {
        /* The task was created.  Use the task's handle to delete the task. */
        //vTaskDelete( xHandle );
    }
#else
    static lv_indev_drv_t indev_drv;

    /*Register a touchpad input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_touchpad = lv_indev_drv_register(&indev_drv);
#endif
}