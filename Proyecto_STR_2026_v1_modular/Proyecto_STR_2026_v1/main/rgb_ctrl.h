#ifndef RGB_CTRL_H
#define RGB_CTRL_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/ledc.h"

typedef struct {
    ledc_mode_t mode;
    ledc_timer_t timer;
    ledc_timer_bit_t resolution;
    uint32_t frequency_hz;
    uint32_t max_duty;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t brightness;
} rgb_ctrl_t;

esp_err_t rgb_ctrl_init(rgb_ctrl_t *rgb);
esp_err_t rgb_ctrl_set(rgb_ctrl_t *rgb, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);

#endif // RGB_CTRL_H
