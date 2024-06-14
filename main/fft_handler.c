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
#if 0
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

            //printf("%fHz (%s%d) - %d (%dHz - %dHz)\n", note_freq, note_names[note], octave, s, (int)round(freq), (int)round(freq + BIN_FREQ ));
            static int last_s = 0;
            static int last_note = 0;
            int t1 = 3;
            int t2 = 5;
            int v = 1 + (s*s*s)/500;

            if ( ((note != last_note) && ( s > last_s + t1)) || ((note == last_note) && ( s > last_s + t2)))
            {
                add_point(0, v, 1, color_map[note], s*2);
            }
            
            last_s = s;
            last_note = note;

        }
    }

    //ESP_LOGI(TAG, "%s",text);
#else
    static uint32_t avg_peak = 100;
    uint32_t bands = 12;
    uint32_t start = (80 / BIN_FREQ);
    uint32_t end = (10000 / BIN_FREQ);
    uint32_t range = (end - start);
    //uint32_t led = 0;
    uint32_t max_peak = 0;

    for (uint32_t b = 0; b < bands; b++)
    {
        uint32_t peak = 0;
        for (uint32_t i = (b * range)/(bands + range); i < ((b+1) * range)/(bands + range); i++)
        {
            if (smoothed[start+i] > 1)
            {
                peak += smoothed[start+i];
            }
        }

        peak = (peak * peak);// / 32;

        if (peak > max_peak)
        {
            max_peak = peak;
        }

        //printf("Band %d - Peak = %d\n", (int)b, (int)peak);

        //printf("Band %d = %d -  %d\n", (int)b, (int)((b * LEDS_COUNT)/(bands)), (int)(((b+1) * LEDS_COUNT)/(bands)));
#if 1
        uint32_t scale = (((1+peak) * 256) / avg_peak);
        scale = (scale < 1) ? 1 : scale;
        for (uint32_t i = 0; i < (peak *LEDS_COUNT)/scale; i++)
        {
            add_to_led((b * LEDS_COUNT)/(bands) + i, color_map[b%12], (peak * 64) / avg_peak);
            //led++;
        }
#else
        for (uint32_t i = 0; i < LEDS_COUNT; i++)
        {
            add_to_led(i, color_map[i%12], 128);
            //led++;
        }
#endif
    }

    //if (max_peak > avg_peak)
    {
        //avg_peak = max_peak;
    }
    //else
    {
        avg_peak = (avg_peak + max_peak) / 2;
        if (avg_peak < 10)
        {
            avg_peak = 10;
        } 
    }

#endif
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