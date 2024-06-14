
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "sdkconfig.h"
#include "driver/i2s_std.h"

#include "guitar_cfg.h"
#include "i2s_handler.h"
#include "fft_handler.h"
#include "wifi_handler.h"

static const char *TAG = "GuitarOS(I2S)";
static i2s_chan_handle_t                rx_chan;
//static i2s_chan_handle_t                tx_chan;

static TaskHandle_t xFftTask = NULL;

#define BUFF_SIZE (SAMPLE_SIZE * 2)

#define BUFFER_COUNT 5
uint8_t buffIndex = 0;
uint8_t r_buf[BUFFER_COUNT][BUFF_SIZE];

__attribute__((aligned(16)))
float sample[SAMPLE_SIZE];

static void i2s_rxtx_task(void *args)
{
    ESP_LOGI(TAG, "i2s_rxtx_task()!");
    size_t r_bytes = 0;
    //size_t t_bytes = 0;


    /* Enable the RX channel */
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
    /* Enable the TX channel */
    //ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
    while (1) {
        /* Read i2s data */
        if (i2s_channel_read(rx_chan, r_buf[buffIndex], BUFF_SIZE, &r_bytes, 500) == ESP_OK) {
            //i2s_channel_write(tx_chan, r_buf[buffIndex], r_bytes, &t_bytes, 500);
            buffIndex = (buffIndex + 1) % BUFFER_COUNT;
            if (xFftTask != NULL) 
            {
                xTaskNotifyGive( xFftTask );
            }
        }
    }

    vTaskDelete(NULL);
}

static void i2s_fft_task(void *args)
{
    ESP_LOGI(TAG, "i2s_fft_task()!");
    while (1) {
        if( ulTaskNotifyTake( pdTRUE, portMAX_DELAY ) == pdTRUE )
        {
            //uint8_t* pBuf = r_buf[(buffIndex + BUFFER_COUNT - 1) % BUFFER_COUNT];
            int16_t* pBuf = r_buf[(buffIndex + BUFFER_COUNT - 1) % BUFFER_COUNT];
            
            for (int i = 0; i < SAMPLE_SIZE; i++)
            {
                //int32_t rawl = (int32_t)(((uint32_t)pBuf[i*6+2] << 16) | ((uint32_t)pBuf[i*6 + 1] << 8) | ((uint32_t)pBuf[i*6 + 0] << 0));
                //int32_t rawr = (int32_t)(((uint32_t)pBuf[i*6+5] << 16) | ((uint32_t)pBuf[i*6 + 4] << 8) | ((uint32_t)pBuf[i*6 + 3] << 0));
                sample[i] = (float)pBuf[i] / (float)0x8000;
            }
            //printf("[0] %10f [1] %10f [2] %10f [3] %10f [4] %10f [5] %10f [6] %10f [7] %10f\n", sample[0], sample[1], sample[2], sample[3], sample[4], sample[5], sample[6], sample[7]);
            run_fft(sample);
        }
    }

    vTaskDelete(NULL);
}

void i2s_init(void)
{
    ESP_LOGI(TAG, "Configuring I2S!");

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    //ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, &rx_chan));
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    i2s_std_config_t rx_std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
    .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
    .gpio_cfg = {
        .mclk = I2S_MCLK_PIN, //I2S_GPIO_UNUSED,    // some codecs may require mclk signal, this example doesn't need it
        .bclk = I2S_BCLK_PIN,
        .ws   = I2S_WS_PIN,
        .dout = I2S_DOUT_PIN,
        .din  = I2S_DIN_PIN,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv   = false,
            },
        },
    };
/*   
    i2s_std_config_t tx_std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
    .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_24BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
        .mclk = I2S_MCLK_PIN, //I2S_GPIO_UNUSED,    // some codecs may require mclk signal, this example doesn't need it
        .bclk = I2S_BCLK_PIN,
        .ws   = I2S_WS_PIN,
        .dout = I2S_DOUT_PIN,
        .din  = I2S_DIN_PIN,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv   = false,
            },
        },
    };
*/

    rx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    rx_std_cfg.slot_cfg.bit_shift = true;
    rx_std_cfg.slot_cfg.big_endian = true;

    //tx_std_cfg.slot_cfg.bit_shift = false;

    //rx_std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384; 
    //tx_std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &rx_std_cfg));
    //ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &tx_std_cfg));
    

    xTaskCreate(i2s_rxtx_task, "i2s_rxtx_task", 2048, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(i2s_fft_task, "i2s_fft_task", 10240, NULL, 5, &xFftTask);
}