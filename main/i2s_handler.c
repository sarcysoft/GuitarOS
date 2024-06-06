
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

#define BUFF_SIZE (SAMPLE_SIZE * 2)

__attribute__((aligned(16)))
float sample[SAMPLE_SIZE];

static void i2s_read_task(void *args)
{
    uint8_t *r_buf = (uint8_t *)calloc(1, BUFF_SIZE);
    assert(r_buf); // Check if r_buf allocation success
    int16_t* pBuf = (int16_t*)r_buf;

    size_t r_bytes = 0;

    /* Enable the RX channel */
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));

    /* ATTENTION: The print and delay in the read task only for monitoring the data by human,
     * Normally there shouldn't be any delays to ensure a short polling time,
     * Otherwise the dma buffer will overflow and lead to the data lost */
    while (1) {
        /* Read i2s data */
        if (i2s_channel_read(rx_chan, r_buf, BUFF_SIZE, &r_bytes, 1000) == ESP_OK) {
            //printf("Read Task: i2s read %d bytes\n-----------------------------------\n", r_bytes);
            /*
            printf("[0] %02x%02x     [1] %02x%02x     [2] %02x%02x     [3] %02x%02x     [4] %02x%02x [5]     %02x%02x [6]     %02x%02x [7]     %02x%02x\n",
                    r_buf[0], r_buf[1], r_buf[2], r_buf[3], r_buf[4], r_buf[5], r_buf[6], r_buf[7],
                    r_buf[8], r_buf[9], r_buf[10], r_buf[11], r_buf[12], r_buf[13], r_buf[14], r_buf[15]);
            printf("%11d %10d %11d %11d %11d %11d %11d %11d\n\n",
                    pBuf[0], pBuf[1], pBuf[2], pBuf[3], pBuf[4], pBuf[5], pBuf[6], pBuf[7]);
                    */
            for (int i = 0; i < SAMPLE_SIZE; i++)
            {
                //int32_t raw = (int32_t)(((uint32_t)r_buf[i*3] << 16) | ((uint32_t)r_buf[i*3 + 1] << 8) | ((uint32_t)r_buf[i*3 + 2] << 0));
                //sample[i] = (float)raw / (float)0x800000;
                sample[i] = (float)pBuf[i*1] / (float)0x8000;
                //int16_t raw = (int16_t)(((uint16_t)r_buf[i*2 + 0] << 8) | ((uint16_t)r_buf[i*2 + 1] << 0));
                //sample[i] = (float)raw / (float)0x8000;
            }
            //printf("[0] %f [1] %f [2] %f [3] %f [4] %f [5] %f [6] %f [7] %f\n", sample[0], sample[1], sample[2], sample[3], sample[4], sample[5], sample[6], sample[7]);
            run_fft(sample);
        } else {
            //printf("Read Task: i2s read failed\n");
        }
        //vTaskDelay(pdMS_TO_TICKS(200));
    }

    vTaskDelete(NULL);
}

void i2s_init(void)
{
    ESP_LOGI(TAG, "Configuring I2S!");

    i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, NULL, &rx_chan));

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
            .bclk_inv = true,
            .ws_inv   = false,
            },
        },
    };

    rx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    //rx_std_cfg.slot_cfg.bit_shift = true;
    //rx_std_cfg.slot_cfg.big_endian = true;
    //rx_std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;
    //rx_std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_EXTERNAL;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &rx_std_cfg));

    xTaskCreate(i2s_read_task, "i2s_read_task", 10240, NULL, 5, NULL);
}