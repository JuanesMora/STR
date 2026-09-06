#ifndef SERVO_CTRL_H
#define SERVO_CTRL_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/ledc.h"
#include "app_state.h"

typedef struct {
    ledc_mode_t mode;
    ledc_channel_t channel;
    ledc_timer_t timer;
    ledc_timer_bit_t resolution;
    uint32_t frequency_hz;
    uint32_t max_duty;
    uint8_t estimated_percent;
} servo_ctrl_t;

esp_err_t servo_ctrl_init(servo_ctrl_t *servo, uint8_t initial_percent);
esp_err_t servo_ctrl_stop(servo_ctrl_t *servo);
esp_err_t servo_ctrl_move_to_percent(servo_ctrl_t *servo, app_context_t *ctx, uint8_t target_percent);

#endif // SERVO_CTRL_H
