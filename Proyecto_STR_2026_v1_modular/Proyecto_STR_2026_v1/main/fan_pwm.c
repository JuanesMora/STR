#include "fan_pwm.h"
#include "app_config.h"
#include "app_state.h"

#include <string.h>
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "fan_pwm";

esp_err_t fan_pwm_init(fan_pwm_t *fan)
{
    if (!fan) return ESP_ERR_INVALID_ARG;
    memset(fan, 0, sizeof(fan_pwm_t));

    fan->mode = APP_LEDC_MODE;
    fan->channel = APP_FAN_LEDC_CHANNEL;
    fan->timer = APP_FAN_LEDC_TIMER;
    fan->resolution = APP_FAN_LEDC_RES;
    fan->frequency_hz = APP_FAN_LEDC_FREQ_HZ;
    fan->max_duty = (1U << fan->resolution) - 1U;
    fan->current_percent = 0;

    ledc_timer_config_t timer_config = {
        .speed_mode = fan->mode,
        .duty_resolution = fan->resolution,
        .timer_num = fan->timer,
        .freq_hz = fan->frequency_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_config), TAG, "fan ledc_timer_config failed");

    ledc_channel_config_t channel_config = {
        .speed_mode = fan->mode,
        .channel = fan->channel,
        .timer_sel = fan->timer,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = APP_GPIO_FAN_PWM,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel_config), TAG, "fan ledc_channel_config failed");

    return fan_pwm_set_percent(fan, 0);
}

esp_err_t fan_pwm_set_percent(fan_pwm_t *fan, uint8_t percent)
{
    if (!fan) return ESP_ERR_INVALID_ARG;
    percent = app_clamp_percent(percent);
    uint32_t duty = (fan->max_duty * percent) / 100U;

    ESP_RETURN_ON_ERROR(ledc_set_duty(fan->mode, fan->channel, duty), TAG, "fan ledc_set_duty failed");
    ESP_RETURN_ON_ERROR(ledc_update_duty(fan->mode, fan->channel), TAG, "fan ledc_update_duty failed");
    fan->current_percent = percent;
    return ESP_OK;
}

uint8_t fan_pwm_compute_auto_percent(float temperature_c, float desired_c, float max_c)
{
    if (max_c <= desired_c) {
        max_c = desired_c + 1.0f;
    }

    if (temperature_c <= desired_c) {
        return 0;
    }
    if (temperature_c >= max_c) {
        return 100;
    }

    float scaled = ((temperature_c - desired_c) * 100.0f) / (max_c - desired_c);
    return app_clamp_percent((int)(scaled + 0.5f));
}
