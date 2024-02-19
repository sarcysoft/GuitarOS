
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
#include "esp_adc/adc_continuous.h"

#include "guitar_cfg.h"
#include "adc_handler.h"
#include "fft_handler.h"
#include "wifi_handler.h"

static const char *TAG = "GuitarOS(ADC)";


static TaskHandle_t s_task_handle;
static adc_channel_t channel[1] = {ADC_CHANNEL_0};


static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = SAMPLE_SIZE * SOC_ADC_DIGI_DATA_BYTES_PER_CONV,
        .conv_frame_size = SAMPLE_SIZE * SOC_ADC_DIGI_DATA_BYTES_PER_CONV,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SAMPLE_RATE,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten = ADC_ATTEN_DB_11;
        adc_pattern[i].channel = channel[i] & 0x7;
        adc_pattern[i].unit = ADC_UNIT_1;
        adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

        ESP_LOGI(TAG, "adc_pattern[%d].atten is :%"PRIx8, i, adc_pattern[i].atten);
        ESP_LOGI(TAG, "adc_pattern[%d].channel is :%"PRIx8, i, adc_pattern[i].channel);
        ESP_LOGI(TAG, "adc_pattern[%d].unit is :%"PRIx8, i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}

uint8_t result[SAMPLE_SIZE*SOC_ADC_DIGI_RESULT_BYTES] = {0};

__attribute__((aligned(16)))
float sampleBuf[SAMPLE_SIZE];
uint8_t rawBuf[SAMPLE_SIZE * 2];

void adc_task( void * pvParameters )
{
    ESP_LOGI(TAG, "adc_task started!");
    s_task_handle = xTaskGetCurrentTaskHandle();

    adc_continuous_handle_t handle = NULL;
    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle);
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle));
    esp_err_t ret;
    uint32_t ret_num = 0;
    memset(result, 0xcc, SAMPLE_SIZE*SOC_ADC_DIGI_RESULT_BYTES);

    ESP_LOGI(TAG, "adc initialised!");

    int32_t baseValue = 0xffffffff;
    int32_t noiseValue = 0;
    int32_t maxVal = 2048;

    while (1) {

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (1) {
            ret = adc_continuous_read(handle, result, SAMPLE_SIZE*SOC_ADC_DIGI_RESULT_BYTES, &ret_num, 0);
            if (ret == ESP_OK)
            {
                if (baseValue == 0xffffffff)
                {
                    uint32_t minV = 0xffffffff;
                    uint32_t maxV = 0;
                    baseValue = 0;
                    for (int i = 0; i < (ret_num / SOC_ADC_DIGI_RESULT_BYTES); i++)
                    {
                        adc_digi_output_data_t *p = (adc_digi_output_data_t*)&result[i*SOC_ADC_DIGI_RESULT_BYTES];

                        /* Check the channel number validation, the data is invalid if the channel num exceed the maximum channel */
                        if (p->type2.channel < SOC_ADC_CHANNEL_NUM(ADC_UNIT_1))
                        {
                            baseValue += p->type2.data;
                        }

                        if (p->type2.data < minV)
                        {
                            minV = p->type2.data;
                        }

                        if (p->type2.data > maxV)
                        {
                            maxV = p->type2.data;
                        }
                    }

                    baseValue /= (ret_num / SOC_ADC_DIGI_RESULT_BYTES);
                    noiseValue = (baseValue - minV) > (maxV - baseValue) ? (baseValue - minV) : (maxV - baseValue);
                    ESP_LOGI(TAG, "ADC base value = %d, noise = %d", (int)baseValue, (int)noiseValue);
                }
                else
                {
                    //ESP_LOGI("TASK", "ret is %x, ret_num is %"PRIu32" bytes", ret, ret_num);
                    for (int i = 0; i < (ret_num / SOC_ADC_DIGI_RESULT_BYTES); i++)
                    {
                        adc_digi_output_data_t *p = (adc_digi_output_data_t*)&result[i*SOC_ADC_DIGI_RESULT_BYTES];

                        /* Check the channel number validation, the data is invalid if the channel num exceed the maximum channel */
                        if (p->type2.channel < SOC_ADC_CHANNEL_NUM(ADC_UNIT_1))
                        {
                            //ESP_LOGI(TAG, "%d - %d", i, (int)(p->type2.data));
                            
                            int32_t iVal = (p->type2.data) - baseValue;
                            rawBuf[i*2] = (iVal >> 0) & 0xFF;
                            rawBuf[i*2+1] = (iVal >> 8) & 0xFF;
                            /*
                            if (iVal > maxVal)
                            {
                                iVal = maxVal;
                            }
                            else if ( iVal < -maxVal)
                            {
                                iVal = -maxVal;
                            }

                            //ESP_LOGI(TAG, "%d - %d", i, (int)iVal);
                            float val = (float)iVal / (float)maxVal;
                            sampleBuf[i] = val;
                            //ESP_LOGW(TAG, "%d - %f", i, sampleBuf[i]);
                            */
                        } 
                        else
                        {

                        }
                    }
    
                    wifi_send_data((uint8_t*)rawBuf, sizeof(rawBuf));
                    //run_fft(sampleBuf);
                }                
            }
            else if (ret == ESP_ERR_TIMEOUT)
            {
                //We try to read `EXAMPLE_READ_LEN` until API returns timeout, which means there's no available data
                //ESP_LOGI(TAG, "adc_continuous_read() == ESP_ERR_TIMEOUT");
                break;
            }
        }
    }

    ESP_ERROR_CHECK(adc_continuous_stop(handle));
    ESP_ERROR_CHECK(adc_continuous_deinit(handle));
}

void adc_init(void)
{
    ESP_LOGI(TAG, "Configuring ADC!");
 BaseType_t xReturned;
TaskHandle_t xHandle = NULL;

    /* Create the task, storing the handle. */
    xReturned = xTaskCreate(
                    adc_task,       /* Function that implements the task. */
                    "ADC Task",          /* Text name for the task. */
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