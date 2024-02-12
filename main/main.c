#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "led_handler.h"
#include "adc_handler.h"
#include "fft_handler.h"
#include "wifi_handler.h"

static const char *TAG = "GuitarOS";

void app_main(void)
{
    ESP_LOGI(TAG, "Start Up!");

    configure_led();   

    adc_init();

    configure_fft();

    wifi_main();

    while (1) {
        vTaskDelay(100);
    }

}
