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

#include "esp_dsp.h"

#include "guitar_cfg.h"
#include "fft_handler.h"
#include "led_handler.h"

static const char *TAG = "GuitarOS(FFT)";

__attribute__((aligned(16)))
float x1[SAMPLE_SIZE * 2];
int32_t smoothed[SAMPLE_SIZE/2];
uint32_t note_map[SAMPLE_SIZE/2];
uint32_t note_strength[MAX_NOTE];
#define SMOOTHING 1

#define BIN_FREQ    (SAMPLE_RATE/SAMPLE_SIZE)

const char* note_names[] = {
    "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"
};

const uint32_t color_map[] = {
    0x00ff0000, 0x00ff8000, 0x00ffff00, 0x0080ff00,
    0x0000ff00, 0x0000ff80, 0x0000ffff, 0x000080ff,
    0x000000ff, 0x008000ff, 0x00ff00ff, 0x00ff0080
};

void update_spectrum(void)
{
    uint32_t i = 1;

    memset(note_strength, 0, sizeof(note_strength));


    for (uint32_t i = 1; i < SAMPLE_SIZE/2; i++)
    {
        uint32_t note = note_map[i];
        if ((note < MAX_NOTE) && (smoothed[i] > 0))
        {
            note_strength[note] += smoothed[i];
        }
    }
    
    //char text[1024] = "";

    uint32_t max_i = 0;

    for (uint32_t i = 0; i < MAX_NOTE; i++)
    {
        //char temp[32];
        //sprintf(temp, "[%s - %3u]", note_names[i], (unsigned int)note_strength[i]);
        //strncat(text, temp, 32);

        if (note_strength[i] > note_strength[max_i])
        {
            max_i = i;
        }
    } 

    if (note_strength[max_i] > 10)
    {
        add_point(0, (int32_t)(note_strength[max_i]/2)+1, 1, color_map[max_i], (note_strength[max_i] / 5)+1);
    }

    //ESP_LOGI(TAG, "%s",text);
}

void process_and_show(float* data, int length)
{
    dsps_fft2r_fc32(data, length);
    // Bit reverse 
    //dsps_bit_rev_fc32(data, length);
    // Convert one complex vector to two complex vectors
    //dsps_cplx2reC_fc32(data, length);

    //for (int i = 0 ; i < length/2 ; i++) {
    //    data[i] = 10 * log10f((data[i * 2 + 0] * data[i * 2 + 0] + data[i * 2 + 1] * data[i * 2 + 1])/SAMPLE_SIZE);
    //}
  
    // Show power spectrum in 64x10 window from -100 to 0 dB from 0..N/4 samples
    //dsps_view(data, length/2, 64, 10,  -120, 40, '|');

    int max_data = 0;
    int max_i = 0;

    for (int i = 0; i < SAMPLE_SIZE/2; i++)
    {
        smoothed[i] += (int)data[i*2];
    }

    static int smooth = 0;

    smooth++;

    if (smooth == SMOOTHING)
    {
        for (int i = 1; i < SAMPLE_SIZE/2; i++)
        {
            if (smoothed[i] > max_data)
            {
                max_data = smoothed[i];
                max_i = i;
            }
        }

#if true
        update_spectrum();
#else
        if (max_data > 5)
        {
            static int last_data = 0;

            if (max_data > last_data)
            {
                int binf = (SAMPLE_RATE / 2) / (SAMPLE_SIZE / 2);
                int freq = binf * max_i + (binf / 2);

                ESP_LOGI(TAG, "Freq %dHz - Val %d", freq, max_data); 

                add_point(0, (int32_t)(max_data/50), (int32_t)(max_data/256), freq, max_data/2);   
            }

            last_data = max_data;
        }
#endif

        smooth = 0;
        memset(smoothed, 0 ,sizeof(smoothed));  
    }
}

void run_fft(float* pSampleBuf)
{
    for (int i = 0 ; i < SAMPLE_SIZE ; i++)
    {
        x1[i*2 + 0] = pSampleBuf[i];
        x1[i*2 + 1] = 0;
    }
    process_and_show(pSampleBuf, SAMPLE_SIZE);
}


void configure_fft()
{
    ESP_LOGI(TAG, "Configuring FFT Handler!");
    esp_err_t ret;
    ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret  != ESP_OK)
    {
        ESP_LOGE(TAG, "Not possible to initialize FFT. Error = %i", ret);
        return;
    }

    memset(smoothed, 0 ,sizeof(smoothed));

    for (uint32_t i = 0; i < SAMPLE_SIZE/2; i++)
    {
        uint32_t freq = (BIN_FREQ * i) + (BIN_FREQ / 2);
        note_map[i] = NOTE_FROM_OCTAVE(NOTE_FROM_FREQ(freq));
    }
}