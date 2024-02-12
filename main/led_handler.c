#include <string.h>
#include <stdio.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

#include "guitar_cfg.h"
#include "led_handler.h"

static const char *TAG = "GuitarOS(LED)";

typedef struct rgb_type {
    uint32_t r;
    uint32_t g;
    uint32_t b;
    uint32_t s;
} rgb_t;

typedef struct light_point_type {
  int32_t loc;
  int32_t vel;
  rgb_t rgb;
  int32_t size;
} light_point_t;

rgb_t led_buf[LEDS_MAX];
light_point_t point_buf[MAX_POINTS];

static led_strip_handle_t led_strip;

void clear_leds()
{
  for (int i = 0; i < LEDS_MAX; i++)
  {
    led_buf[i].r = 0;
    led_buf[i].g = 0;
    led_buf[i].b = 0;
    led_buf[i].s = 0;
  }
}

void clear_points()
{

  for (int i = 0; i < MAX_POINTS; i++)
  {
    point_buf[i].loc = -1;
  }
}

void update_pts()
{
  clear_leds();

  for (int i = 0; i < MAX_POINTS; i++)
  {
    if (point_buf[i].loc >= 0)
    {
      int32_t loc = point_buf[i].loc + point_buf[i].vel;

      if ((loc >= 0) && (loc < LEDS_MAX))
      {
        int32_t s = point_buf[i].size;
        for (int l = loc - s; l <= loc + s; l++)
        {
          if ((l >= 0) && (l < LEDS_MAX))
          {
            rgb_t rgb = point_buf[i].rgb;
            int32_t x = s - abs(l - loc);

            led_buf[l].r = (x * rgb.r) / s;
            led_buf[l].g = (x * rgb.g) / s;
            led_buf[l].b = (x * rgb.b) / s;

            led_buf[l].s++;
          }
        }

        point_buf[i].loc = loc;
      }
      else
      {
        point_buf[i].loc = -1;
      }
    }
  }
}

const double Gamma = 0.80;
const double IntensityMax = 255;


rgb_t waveLengthToRGB(double Wavelength, int8_t stren)
{
    double factor;
    double Red, Green, Blue;

    if((Wavelength >= 380) && (Wavelength < 440)) {
        Red = -(Wavelength - 440) / (440 - 380);
        Green = 0.0;
        Blue = 1.0;
    } else if((Wavelength >= 440) && (Wavelength < 490)) {
        Red = 0.0;
        Green = (Wavelength - 440) / (490 - 440);
        Blue = 1.0;
    } else if((Wavelength >= 490) && (Wavelength < 510)) {
        Red = 0.0;
        Green = 1.0;
        Blue = -(Wavelength - 510) / (510 - 490);
    } else if((Wavelength >= 510) && (Wavelength < 580)) {
        Red = (Wavelength - 510) / (580 - 510);
        Green = 1.0;
        Blue = 0.0;
    } else if((Wavelength >= 580) && (Wavelength < 645)) {
        Red = 1.0;
        Green = -(Wavelength - 645) / (645 - 580);
        Blue = 0.0;
    } else if((Wavelength >= 645) && (Wavelength < 781)) {
        Red = 1.0;
        Green = 0.0;
        Blue = 0.0;
    } else {
        Red = 0.0;
        Green = 0.0;
        Blue = 0.0;
    }

    // Let the intensity fall off near the vision limits

    if((Wavelength >= 380) && (Wavelength < 420)) {
        factor = 0.3 + 0.7 * (Wavelength - 380) / (420 - 380);
    } else if((Wavelength >= 420) && (Wavelength < 701)) {
        factor = 1.0;
    } else if((Wavelength >= 701) && (Wavelength < 781)) {
        factor = 0.3 + 0.7 * (780 - Wavelength) / (780 - 700);
    } else {
        factor = 0.0;
    }


    rgb_t rgb;

    // Don't want 0^x = 1 for x <> 0
    rgb.r = Red == 0.0 ? 0 : (int)round(stren * pow(Red * factor, Gamma));
    rgb.g = Green == 0.0 ? 0 : (int)round(stren * pow(Green * factor, Gamma));
    rgb.b = Blue == 0.0 ? 0 : (int)round(stren * pow(Blue * factor, Gamma));

    return rgb;
}

rgb_t col_to_rgb(uint32_t col, uint32_t stren)
{
    rgb_t rgb;
    
    stren = stren < 256 ? stren : 256;

    rgb.r = ((stren  * (col >> 0)) / 256) & 0xff;
    rgb.g = ((stren  * (col >> 8)) / 256) & 0xff;
    rgb.b = ((stren  * (col >> 16)) / 256) & 0xff;

    return rgb;
}

int32_t map_freq_to_wl(int32_t freq)
{
  int32_t wl = VISIBLE_WL_MIN;

  if (freq <= GUITAR_FREQ_MIN)
  {
    wl = VISIBLE_WL_MAX;
  }
  else if (freq >= GUITAR_FREQ_MAX)
  {
    wl = VISIBLE_WL_MIN;
  }
  else
  {
    wl = VISIBLE_WL_MAX - (((freq - GUITAR_FREQ_MIN) * VISIBLE_WL_RANGE)/GUITAR_FREQ_RANGE);
  }

  return wl;
}

void add_point(int32_t loc, int32_t vel, int32_t sz, uint32_t col, uint32_t stren)
{
  for (int i = 0; i < MAX_POINTS; i++)
  {
    if (point_buf[i].loc < 0)
    {
      point_buf[i].loc = loc;
      point_buf[i].vel = vel;
      point_buf[i].rgb = col_to_rgb(col, stren);

      point_buf[i].size = sz + 1;
      break;
    }
  }
}
 

void update_leds( TimerHandle_t xTimer )
{
  update_pts();

  for (int i = 0; i < LEDS_COUNT; i++) {
    rgb_t rgb = {.r = 0, .g = 0, .b = 0};

    int t = 0;

    for (int j = 1-LEDS_ALIAS; j < LEDS_ALIAS; j++)
    {
      int k = (i * LEDS_ALIAS) + j;
      if ((k > 0) && (led_buf[k].s > 0))
      {
        int s = LEDS_ALIAS - abs(j);
        rgb.r += (s * led_buf[k].r)/led_buf[k].s;
        rgb.g += (s * led_buf[k].g)/led_buf[k].s;
        rgb.b += (s * led_buf[k].b)/led_buf[k].s;
        t  += s;
      }
    }
    if (t > 0)
    {
        led_strip_set_pixel(led_strip, i, rgb.r/t, rgb.g/t, rgb.b/t);
    }
    else
    {
        led_strip_set_pixel(led_strip, i, 0, 0, 0);
    }
  }

  led_strip_refresh(led_strip);
}

void configure_led(void)
{
    ESP_LOGI(TAG, "Configuring LEDs!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = LEDS_PIN,
        .max_leds = LEDS_COUNT, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 12 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);

    clear_leds();
    clear_points();

    TimerHandle_t hUpdate = xTimerCreate("UpdateLEDs",
                                    25 / portTICK_PERIOD_MS,
                                    pdTRUE,
                                    ( void * ) 0,
                                    update_leds);

    ESP_LOGI(TAG, "Starting LED update timer");
    xTimerStart(hUpdate, 0);
}