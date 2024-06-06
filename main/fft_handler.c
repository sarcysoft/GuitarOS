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
float wind[SAMPLE_SIZE];
__attribute__((aligned(16)))
float y_cf[SAMPLE_SIZE * 2];

int32_t smoothed[SAMPLE_SIZE/2];

#define BIN_FREQ    ((double)SAMPLE_RATE/((double)SAMPLE_SIZE/1.0))

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
    //ESP_LOGI(TAG, "update_spectrum()");
    uint32_t max_i = 0;

    for (uint32_t i = (GUITAR_FREQ_MIN / BIN_FREQ); i < (GUITAR_FREQ_MAX / BIN_FREQ); i++)
    {
        uint32_t freq = (int)((BIN_FREQ * i) - (BIN_FREQ / 2));
        if (i < SAMPLE_SIZE/2)
        {
            if (smoothed[i] > smoothed[max_i])
            {
                max_i = i;
            }
        }
        else
        {
            break;
        }
    }

    if ((max_i > 1) && (smoothed[max_i] > 0))
    {
        double freq = (BIN_FREQ * max_i);
        double lower = (double)smoothed[max_i - 1];
        double upper = (double)smoothed[max_i + 1];
        double offset = 1;
        if (upper != lower)
        {
            offset += ((upper - lower) / (upper + lower)) * ((upper+lower) / (2 * smoothed[max_i]));
        }

        double note_freq = freq + (BIN_FREQ * (offset / 2));
        if (note_freq > 20)
        {
            int s = smoothed[max_i];
            int octave = OCTAVE_FROM_NOTE(NOTE_FROM_FREQ(note_freq));
            int note = NOTE_FROM_OCTAVE(NOTE_FROM_FREQ(note_freq));
#if 1
            //printf("%fHz (%s%d) - %d (%dHz - %dHz)\n", note_freq, note_names[note], octave, s, (int)round(freq), (int)round(freq + BIN_FREQ ));
            static int last_v = 0;
            static int last_note = 0;
            int v = 1 + (10 * s)/100;

            if ( (abs(note - last_note) > 1) || ( v > last_v))
            {
                add_point(0, v, v, color_map[note], (s*5));
            }
            
            last_v = v;
            last_note = note;
#else
            int v = 1 + (32 * s)/100;
            double wl = VISIBLE_WL_MIN + VISIBLE_WL_RANGE * ((note_freq - GUITAR_FREQ_MIN) / GUITAR_FREQ_RANGE);
            add_point(0, v, v, waveLengthToCol(wl, 128), (s*8));
#endif
        }
    }

    //ESP_LOGI(TAG, "%s",text);
}

void process_and_show(float* data, int length)
{
    //ESP_LOGI(TAG, "process_and_show()");
    dsps_fft2r_fc32(data, length);

    // Bit reverse 
    dsps_bit_rev_fc32(data, length);

    // Convert one complex vector to two complex vectors
    dsps_cplx2reC_fc32(data, length);

    for (int i = 0 ; i < length/2 ; i++) {
        data[i] = 10 * log10f((data[i * 2 + 0] * data[i * 2 + 0] + data[i * 2 + 1] * data[i * 2 + 1])/SAMPLE_SIZE);
    }

    //printf("[0] %f [1] %f [2] %f [3] %f [4] %f [5] %f [6] %f [7] %f\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
  
    // Show power spectrum in 64x10 window from -100 to 0 dB from 0..N/4 samples
    //dsps_view(data, length/2, 64, 10,  -120, 40, '|');

    for (int i = 0; i < length/2; i++)
    {
        smoothed[i] = round(data[i]);
    }
    //printf("[0] %d [1] %d [2] %d [3] %d [4] %d [5] %d [6] %d [7] %d\n", (int)smoothed[0], (int)smoothed[1], (int)smoothed[2], (int)smoothed[3], (int)smoothed[4], (int)smoothed[5], (int)smoothed[6], (int)smoothed[7]);

     update_spectrum();
}

void run_fft(float* x1)
{
    //ESP_LOGI(TAG, "run_fft()");
    dsps_wind_hann_f32(wind, SAMPLE_SIZE);

    for (int i = 0 ; i < SAMPLE_SIZE ; i++)
    {
        y_cf[i*2 + 0] = x1[i] * wind[i];
        y_cf[i*2 + 1] = 0;
    }
    process_and_show(y_cf, SAMPLE_SIZE);
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

}