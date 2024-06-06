#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "led_handler.h"
#include "i2s_handler.h"
//#include "adc_handler.h"
#include "fft_handler.h"
#include "wifi_handler.h"
#include "lcd_handler.h"
#include "touch_handler.h"
#include "mems_handler.h"

static const char *TAG = "GuitarOS";

void app_main(void)
{
    ESP_LOGI(TAG, "Start Up!");

    configure_lcd();   

    configure_touch();
    
    configure_led();   

    configure_mems();  


    configure_fft();
    
    //adc_init();
    i2s_init(); 

    //wifi_init();

    //while (!wifi_is_connected());
    //wifi_open_socket();

    while (1) {
        vTaskDelay(100);
    }

}
