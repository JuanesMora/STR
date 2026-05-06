#ifndef LIBRARY_LEDC_H
#define LIBRARY_LEDC_H

#include "driver/ledc.h"
#include "esp_err.h"

typedef struct {
    int gpio_num;
    ledc_channel_t channel;
    uint32_t duty;
} led_channel_t;

typedef struct {
    ledc_mode_t speed_mode;
    ledc_timer_t timer;
    ledc_timer_bit_t duty_resolution;
    uint32_t frequency;

    led_channel_t led_red;
    led_channel_t led_green;
    led_channel_t led_blue;

} led_rgb_t;

void config_led_rgb(led_rgb_t *led_rgb);
void set_led_rgb_given_struct(led_rgb_t *led_rgb);
void set_led_rgb_given_values(led_rgb_t *led_rgb, uint32_t duty_red, uint32_t duty_green, uint32_t duty_blue);
void set_led_rgb_percentage_given_values(led_rgb_t *led_rgb, int percentage_red, int percentage_green, int percentage_blue);

#endif