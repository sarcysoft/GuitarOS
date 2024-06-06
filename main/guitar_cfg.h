#ifndef GUITAR_CFG_H
#define GUITAR_CFG_H

// Visible: 380nm - 780nm
#define VISIBLE_WL_MIN     380
#define VISIBLE_WL_MAX     780
#define VISIBLE_WL_RANGE   (VISIBLE_WL_MAX - VISIBLE_WL_MIN)

#define LEDS_PIN     33
#define LEDS_COUNT   50
#define LEDS_ALIAS   10
#define LEDS_MAX     (LEDS_COUNT * LEDS_ALIAS)

#define I2S_MCLK_PIN 18
#define I2S_BCLK_PIN 15
#define I2S_DOUT_PIN 21
#define I2S_DIN_PIN  16
#define I2S_WS_PIN   17

#define MAX_POINTS 50

// Guitar: 80Hz - 7KHz
#define GUITAR_FREQ_MIN     80
#define GUITAR_FREQ_MAX     2000
#define GUITAR_FREQ_RANGE   (GUITAR_FREQ_MAX - GUITAR_FREQ_MIN)

#define SAMPLE_SIZE (4096)
#define SAMPLE_RATE (96000)


#define NOTE_A4  440.0

#define NOTE_FROM_FREQ(fn)      ((int)round((12*log2((double)fn/27.5))))
#define FREQ_FROM_NOTE(n)       (pow(2,(double)n/12)*27.5)
#define OCTAVE_FROM_NOTE(n)     ((n+9)/12)
#define NOTE_FROM_OCTAVE(n)     (n%12)


#define MAX_NOTE        12

#endif