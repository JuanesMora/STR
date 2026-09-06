#include "app_tasks.h"
#include "app_config.h"
#include "ntc_adc.h"
#include "fan_pwm.h"
#include "rgb_ctrl.h"
#include "servo_ctrl.h"
#include "alarm_led.h"
#include "wifi_app.h"

#include <stdlib.h>
#include <time.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app_tasks";

typedef struct {
    app_context_t *ctx;
    ntc_adc_t ntc;
    fan_pwm_t fan;
    rgb_ctrl_t rgb;
    servo_ctrl_t servo;
} app_runtime_t;

static void task_temperature(void *pvParameters)
{
    app_runtime_t *rt = (app_runtime_t *)pvParameters;

    while (1) {
        int raw = 0;
        float voltage = 0.0f;
        float temp_c = 0.0f;

        esp_err_t err = ntc_adc_read(&rt->ntc, &raw, &voltage, &temp_c);
        if (err == ESP_OK) {
            app_state_set_temperature(rt->ctx, temp_c, raw, voltage);
            ESP_LOGI(TAG, "Temperature: %.2f C | ADC: %d | %.3f V", temp_c, raw, voltage);
        } else {
            ESP_LOGW(TAG, "Temperature read failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(APP_TASK_TEMP_PERIOD_MS));
    }
}

static void task_fan_control(void *pvParameters)
{
    app_runtime_t *rt = (app_runtime_t *)pvParameters;

    while (1) {
        app_state_t state;
        if (app_state_get_snapshot(rt->ctx, &state)) {
            uint8_t percent = state.fan_percent;

            if (state.fan_mode == APP_FAN_MODE_AUTO) {
                percent = fan_pwm_compute_auto_percent(state.temperature_c, state.temp_desired_c, state.temp_max_c);
                app_state_set_fan_percent(rt->ctx, percent);
            }

            fan_pwm_set_percent(&rt->fan, percent);
            app_state_set_alarm(rt->ctx, state.temperature_c > state.temp_max_c);
        }

        vTaskDelay(pdMS_TO_TICKS(APP_TASK_FAN_PERIOD_MS));
    }
}

static void task_rgb_control(void *pvParameters)
{
    app_runtime_t *rt = (app_runtime_t *)pvParameters;

    while (1) {
        app_state_t state;
        if (app_state_get_snapshot(rt->ctx, &state)) {
            rgb_ctrl_set(&rt->rgb, state.rgb_r, state.rgb_g, state.rgb_b, state.rgb_brightness);
        }
        vTaskDelay(pdMS_TO_TICKS(APP_TASK_RGB_PERIOD_MS));
    }
}

static void task_servo_control(void *pvParameters)
{
    app_runtime_t *rt = (app_runtime_t *)pvParameters;
    servo_command_t command;

    while (1) {
        if (xQueueReceive(rt->ctx->servo_queue, &command, portMAX_DELAY) == pdTRUE) {
            servo_ctrl_move_to_percent(&rt->servo, rt->ctx, command.target_percent);
        }
    }
}

static void task_alarm_led(void *pvParameters)
{
    app_runtime_t *rt = (app_runtime_t *)pvParameters;
    bool blink_state = false;

    while (1) {
        app_state_t state;
        if (app_state_get_snapshot(rt->ctx, &state) && state.alarm_active) {
            blink_state = !blink_state;
            alarm_led_set(blink_state);
        } else {
            blink_state = false;
            alarm_led_set(false);
        }

        vTaskDelay(pdMS_TO_TICKS(APP_TASK_ALARM_PERIOD_MS));
    }
}

static void task_schedule_control(void *pvParameters)
{
    app_runtime_t *rt = (app_runtime_t *)pvParameters;
    int last_executed_minute = -1;

    while (1) {
        if (get_state_time_was_synchronized()) {
            time_t now;
            struct tm timeinfo;

            if (time(&now) != -1 && localtime_r(&now, &timeinfo) != NULL) {
                int current_day_minute = (timeinfo.tm_yday * 24 * 60) + (timeinfo.tm_hour * 60) + timeinfo.tm_min;

                app_state_t state;
                if (app_state_get_snapshot(rt->ctx, &state) && state.curtain_mode == APP_CURTAIN_MODE_AUTO) {
                    for (uint8_t i = 0; i < APP_CURTAIN_SCHEDULE_RECORDS; i++) {
                        curtain_schedule_record_t rec = state.schedule[i];
                        if (rec.enabled && rec.hour == timeinfo.tm_hour && rec.minute == timeinfo.tm_min) {
                            if (last_executed_minute != current_day_minute) {
                                ESP_LOGI(TAG, "Executing curtain schedule %u -> %u%%", i, rec.percent);
                                app_context_send_servo_command(rt->ctx, rec.percent, true);
                                last_executed_minute = current_day_minute;
                            }
                        }
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(APP_TASK_SCHEDULE_PERIOD_MS));
    }
}

esp_err_t app_tasks_start(app_context_t *ctx)
{
    if (!ctx) return ESP_ERR_INVALID_ARG;

    app_runtime_t *rt = calloc(1, sizeof(app_runtime_t));
    if (!rt) return ESP_ERR_NO_MEM;

    rt->ctx = ctx;

    ESP_ERROR_CHECK(ntc_adc_init(&rt->ntc));
    ESP_ERROR_CHECK(fan_pwm_init(&rt->fan));
    ESP_ERROR_CHECK(rgb_ctrl_init(&rt->rgb));
    ESP_ERROR_CHECK(servo_ctrl_init(&rt->servo, APP_DEFAULT_CURTAIN_PERCENT));
    ESP_ERROR_CHECK(alarm_led_init());

    xTaskCreate(task_temperature, "task_temperature", APP_TASK_TEMP_STACK, rt, APP_TASK_TEMP_PRIORITY, NULL);
    xTaskCreate(task_fan_control, "task_fan_control", APP_TASK_FAN_STACK, rt, APP_TASK_FAN_PRIORITY, NULL);
    xTaskCreate(task_rgb_control, "task_rgb_control", APP_TASK_RGB_STACK, rt, APP_TASK_RGB_PRIORITY, NULL);
    xTaskCreate(task_servo_control, "task_servo_control", APP_TASK_SERVO_STACK, rt, APP_TASK_SERVO_PRIORITY, NULL);
    xTaskCreate(task_alarm_led, "task_alarm_led", APP_TASK_ALARM_STACK, rt, APP_TASK_ALARM_PRIORITY, NULL);
    xTaskCreate(task_schedule_control, "task_schedule_control", APP_TASK_SCHEDULE_STACK, rt, APP_TASK_SCHEDULE_PRIORITY, NULL);

    return ESP_OK;
}
