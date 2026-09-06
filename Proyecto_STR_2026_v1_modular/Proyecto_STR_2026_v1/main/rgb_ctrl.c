#include "rgb_ctrl.h"
#include "app_config.h"
#include "app_state.h"

#include <string.h>
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "rgb_ctrl";

static uint32_t rgb_percent_to_duty(rgb_ctrl_t *rgb, uint8_t color_percent, uint8_t brightness_percent)
{
    uint8_t color = app_clamp_percent(color_percent);
    uint8_t brightness = app_clamp_percent(brightness_percent);
    uint32_t logical_duty = (rgb->max_duty * color * brightness) / 10000U;

#if APP_RGB_COMMON_CATHODE
    return logical_duty;
#else
    return rgb->max_duty - logical_duty;
#endif
}

static esp_err_t rgb_config_channel(rgb_ctrl_t *rgb, ledc_channel_t channel, gpio_num_t gpio)
{
    ledc_channel_config_t channel_config = {
        .speed_mode = rgb->mode,
        .channel = channel,
        .timer_sel = rgb->timer,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = gpio,
#if APP_RGB_COMMON_CATHODE
        .duty = 0,
#else
        .duty = (1U << APP_RGB_LEDC_RES) - 1U,
#endif
        .hpoint = 0,
    };
    return ledc_channel_config(&channel_config);
}

esp_err_t rgb_ctrl_init(rgb_ctrl_t *rgb)
{
    if (!rgb) return ESP_ERR_INVALID_ARG;
    memset(rgb, 0, sizeof(rgb_ctrl_t));

    rgb->mode = APP_LEDC_MODE;
    rgb->timer = APP_RGB_LEDC_TIMER;
    rgb->resolution = APP_RGB_LEDC_RES;
    rgb->frequency_hz = APP_RGB_LEDC_FREQ_HZ;
    rgb->max_duty = (1U << rgb->resolution) - 1U;

    ledc_timer_config_t timer_config = {
        .speed_mode = rgb->mode,
        .duty_resolution = rgb->resolution,
        .timer_num = rgb->timer,
        .freq_hz = rgb->frequency_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_config), TAG, "rgb ledc_timer_config failed");
    ESP_RETURN_ON_ERROR(rgb_config_channel(rgb, APP_RGB_CH_RED, APP_GPIO_RGB_RED), TAG, "rgb red channel failed");
    ESP_RETURN_ON_ERROR(rgb_config_channel(rgb, APP_RGB_CH_GREEN, APP_GPIO_RGB_GREEN), TAG, "rgb green channel failed");
    ESP_RETURN_ON_ERROR(rgb_config_channel(rgb, APP_RGB_CH_BLUE, APP_GPIO_RGB_BLUE), TAG, "rgb blue channel failed");

    return rgb_ctrl_set(rgb, APP_DEFAULT_RGB_R, APP_DEFAULT_RGB_G, APP_DEFAULT_RGB_B, APP_DEFAULT_RGB_BRIGHTNESS);
}

esp_err_t rgb_ctrl_set(rgb_ctrl_t *rgb, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    if (!rgb) return ESP_ERR_INVALID_ARG;

    r = app_clamp_percent(r);
    g = app_clamp_percent(g);
    b = app_clamp_percent(b);
    brightness = app_clamp_percent(brightness);

    uint32_t duty_r = rgb_percent_to_duty(rgb, r, brightness);
    uint32_t duty_g = rgb_percent_to_duty(rgb, g, brightness);
    uint32_t duty_b = rgb_percent_to_duty(rgb, b, brightness);

    ESP_RETURN_ON_ERROR(ledc_set_duty(rgb->mode, APP_RGB_CH_RED, duty_r), TAG, "rgb red duty failed");
    ESP_RETURN_ON_ERROR(ledc_update_duty(rgb->mode, APP_RGB_CH_RED), TAG, "rgb red update failed");

    ESP_RETURN_ON_ERROR(ledc_set_duty(rgb->mode, APP_RGB_CH_GREEN, duty_g), TAG, "rgb green duty failed");
    ESP_RETURN_ON_ERROR(ledc_update_duty(rgb->mode, APP_RGB_CH_GREEN), TAG, "rgb green update failed");

    ESP_RETURN_ON_ERROR(ledc_set_duty(rgb->mode, APP_RGB_CH_BLUE, duty_b), TAG, "rgb blue duty failed");
    ESP_RETURN_ON_ERROR(ledc_update_duty(rgb->mode, APP_RGB_CH_BLUE), TAG, "rgb blue update failed");

    rgb->r = r;
    rgb->g = g;
    rgb->b = b;
    rgb->brightness = brightness;

    return ESP_OK;
}
