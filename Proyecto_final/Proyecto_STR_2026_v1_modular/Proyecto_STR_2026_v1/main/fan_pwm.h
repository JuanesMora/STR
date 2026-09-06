#ifndef FAN_PWM_H
#define FAN_PWM_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/ledc.h"

typedef struct {
    ledc_mode_t mode;
    ledc_channel_t channel;
    ledc_timer_t timer;
    ledc_timer_bit_t resolution;
    uint32_t frequency_hz;
    uint32_t max_duty;
    uint8_t current_percent;
} fan_pwm_t;

esp_err_t fan_pwm_init(fan_pwm_t *fan);
esp_err_t fan_pwm_set_percent(fan_pwm_t *fan, uint8_t percent);
uint8_t fan_pwm_compute_auto_percent(float temperature_c, float desired_c, float max_c);

#endif // FAN_PWM_H
