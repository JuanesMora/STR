#include "servo_ctrl.h"
#include "app_config.h"

#include <stdlib.h>
#include <string.h>
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "servo_ctrl";

static uint32_t servo_pulse_us_to_duty(servo_ctrl_t *servo, uint32_t pulse_us)
{
    const uint32_t period_us = 1000000UL / servo->frequency_hz;
    return (servo->max_duty * pulse_us) / period_us;
}

static esp_err_t servo_set_pulse_us(servo_ctrl_t *servo, uint32_t pulse_us)
{
    uint32_t duty = servo_pulse_us_to_duty(servo, pulse_us);
    ESP_RETURN_ON_ERROR(ledc_set_duty(servo->mode, servo->channel, duty), TAG, "servo duty failed");
    ESP_RETURN_ON_ERROR(ledc_update_duty(servo->mode, servo->channel), TAG, "servo update failed");
    return ESP_OK;
}

esp_err_t servo_ctrl_init(servo_ctrl_t *servo, uint8_t initial_percent)
{
    if (!servo) return ESP_ERR_INVALID_ARG;
    memset(servo, 0, sizeof(servo_ctrl_t));

    servo->mode = APP_LEDC_MODE;
    servo->channel = APP_SERVO_LEDC_CHANNEL;
    servo->timer = APP_SERVO_LEDC_TIMER;
    servo->resolution = APP_SERVO_LEDC_RES;
    servo->frequency_hz = APP_SERVO_LEDC_FREQ_HZ;
    servo->max_duty = (1U << servo->resolution) - 1U;
    servo->estimated_percent = app_clamp_percent(initial_percent);

    ledc_timer_config_t timer_config = {
        .speed_mode = servo->mode,
        .duty_resolution = servo->resolution,
        .timer_num = servo->timer,
        .freq_hz = servo->frequency_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_config), TAG, "servo timer failed");

    ledc_channel_config_t channel_config = {
        .speed_mode = servo->mode,
        .channel = servo->channel,
        .timer_sel = servo->timer,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = APP_GPIO_SERVO_PWM,
        .duty = servo_pulse_us_to_duty(servo, APP_SERVO_STOP_US),
        .hpoint = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel_config), TAG, "servo channel failed");

    return servo_ctrl_stop(servo);
}

esp_err_t servo_ctrl_stop(servo_ctrl_t *servo)
{
    if (!servo) return ESP_ERR_INVALID_ARG;
    return servo_set_pulse_us(servo, APP_SERVO_STOP_US);
}

esp_err_t servo_ctrl_move_to_percent(servo_ctrl_t *servo, app_context_t *ctx, uint8_t target_percent)
{
    if (!servo || !ctx) return ESP_ERR_INVALID_ARG;

    target_percent = app_clamp_percent(target_percent);
    int diff = (int)target_percent - (int)servo->estimated_percent;

    if (diff == 0) {
        app_state_set_curtain_percent(ctx, target_percent);
        return servo_ctrl_stop(servo);
    }

    uint32_t pulse = diff > 0 ? APP_SERVO_OPEN_US : APP_SERVO_CLOSE_US;
    uint32_t move_ms = (APP_SERVO_FULL_TRAVEL_MS * (uint32_t)abs(diff)) / 100U;

    ESP_LOGI(TAG, "Moving curtain from %u%% to %u%% for %lu ms", servo->estimated_percent, target_percent, (unsigned long)move_ms);

    ESP_RETURN_ON_ERROR(servo_set_pulse_us(servo, pulse), TAG, "servo start move failed");
    vTaskDelay(pdMS_TO_TICKS(move_ms));
    ESP_RETURN_ON_ERROR(servo_ctrl_stop(servo), TAG, "servo stop failed");
    vTaskDelay(pdMS_TO_TICKS(APP_SERVO_SETTLE_MS));

    servo->estimated_percent = target_percent;
    app_state_set_curtain_percent(ctx, target_percent);

    return ESP_OK;
}
