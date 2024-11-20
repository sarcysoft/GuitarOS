#ifndef GUITAR_CFG_H
#define GUITAR_CFG_H

#define DATA_YELLOW 0

#define DATA_BROWN  15
#define DATA_ORANGE 16
#define DATA_GREEN  17
#define DATA_BLUE   18
#define DATA_PURPLE 21
#define DATA_GREY   33


// Visible: 380nm - 780nm
#define VISIBLE_WL_MIN     380
#define VISIBLE_WL_MAX     780
#define VISIBLE_WL_RANGE   (VISIBLE_WL_MAX - VISIBLE_WL_MIN)

#define LEDS_PIN     DATA_GREY
#define LEDS_COUNT   50
#define LEDS_ALIAS   15
#define LEDS_MAX     (LEDS_COUNT * LEDS_ALIAS)

#define I2S_MCLK_PIN I2S_GPIO_UNUSED
#define I2S_BCLK_PIN DATA_BROWN
#define I2S_DIN_PIN  DATA_ORANGE
#define I2S_DOUT_PIN I2S_GPIO_UNUSED
#define I2S_WS_PIN   DATA_GREEN

#define ENC_PIN_D0  DATA_BLUE
#define ENC_PIN_D1  DATA_PURPLE
#define ENC_PIN_SW  DATA_YELLOW

#define MAX_POINTS 50

// Guitar: 80Hz - 7KHz
#define GUITAR_FREQ_MIN     80
#define GUITAR_FREQ_MAX     2000
#define GUITAR_FREQ_RANGE   (GUITAR_FREQ_MAX - GUITAR_FREQ_MIN)

#define SAMPLE_SIZE (2*1024)
#define SAMPLE_RATE (96000)


#define NOTE_A4  440.0

#define NOTE_FROM_FREQ(fn)      ((int)round((12*log2((double)fn/27.5))))
#define FREQ_FROM_NOTE(n)       (pow(2,(double)n/12)*27.5)
#define OCTAVE_FROM_NOTE(n)     ((n+9)/12)
#define NOTE_FROM_OCTAVE(n)     (n%12)


#define MAX_NOTE        12

#endif