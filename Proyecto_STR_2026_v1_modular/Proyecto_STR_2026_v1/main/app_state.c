#include "app_state.h"
#include <string.h>

uint8_t app_clamp_percent(int value)
{
    if (value < 0) return 0;
    if (value > 100) return 100;
    return (uint8_t)value;
}

void app_context_init(app_context_t *ctx)
{
    memset(ctx, 0, sizeof(app_context_t));

    ctx->mutex = xSemaphoreCreateMutex();
    ctx->servo_queue = xQueueCreate(8, sizeof(servo_command_t));

    ctx->state.temperature_c = 0.0f;
    ctx->state.adc_raw = 0;
    ctx->state.adc_voltage_v = 0.0f;

    ctx->state.temp_desired_c = APP_DEFAULT_TEMP_DESIRED_C;
    ctx->state.temp_max_c = APP_DEFAULT_TEMP_MAX_C;
    ctx->state.fan_mode = APP_DEFAULT_FAN_AUTO_MODE ? APP_FAN_MODE_AUTO : APP_FAN_MODE_MANUAL;
    ctx->state.fan_percent = APP_DEFAULT_FAN_PERCENT;

    ctx->state.rgb_r = APP_DEFAULT_RGB_R;
    ctx->state.rgb_g = APP_DEFAULT_RGB_G;
    ctx->state.rgb_b = APP_DEFAULT_RGB_B;
    ctx->state.rgb_brightness = APP_DEFAULT_RGB_BRIGHTNESS;

    ctx->state.curtain_mode = APP_DEFAULT_CURTAIN_AUTO_MODE ? APP_CURTAIN_MODE_AUTO : APP_CURTAIN_MODE_MANUAL;
    ctx->state.curtain_percent = APP_DEFAULT_CURTAIN_PERCENT;
    ctx->state.alarm_active = false;

    for (uint8_t i = 0; i < APP_CURTAIN_SCHEDULE_RECORDS; i++) {
        ctx->state.schedule[i].enabled = false;
        ctx->state.schedule[i].hour = 99;
        ctx->state.schedule[i].minute = 99;
        ctx->state.schedule[i].percent = 0;
    }
}

bool app_state_get_snapshot(app_context_t *ctx, app_state_t *out_state)
{
    if (!ctx || !out_state || !ctx->mutex) return false;
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    *out_state = ctx->state;
    xSemaphoreGive(ctx->mutex);
    return true;
}

bool app_state_set_temperature(app_context_t *ctx, float temperature_c, int adc_raw, float voltage_v)
{
    if (!ctx || !ctx->mutex) return false;
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    ctx->state.temperature_c = temperature_c;
    ctx->state.adc_raw = adc_raw;
    ctx->state.adc_voltage_v = voltage_v;
    xSemaphoreGive(ctx->mutex);
    return true;
}

bool app_state_set_fan_config(app_context_t *ctx, app_fan_mode_t mode, float desired_c, float max_c, uint8_t manual_percent)
{
    if (!ctx || !ctx->mutex) return false;
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;

    ctx->state.fan_mode = mode;
    ctx->state.temp_desired_c = desired_c;
    ctx->state.temp_max_c = max_c;
    if (mode == APP_FAN_MODE_MANUAL) {
        ctx->state.fan_percent = app_clamp_percent(manual_percent);
    }

    xSemaphoreGive(ctx->mutex);
    return true;
}

bool app_state_set_fan_percent(app_context_t *ctx, uint8_t fan_percent)
{
    if (!ctx || !ctx->mutex) return false;
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    ctx->state.fan_percent = app_clamp_percent(fan_percent);
    xSemaphoreGive(ctx->mutex);
    return true;
}

bool app_state_set_rgb(app_context_t *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    if (!ctx || !ctx->mutex) return false;
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    ctx->state.rgb_r = app_clamp_percent(r);
    ctx->state.rgb_g = app_clamp_percent(g);
    ctx->state.rgb_b = app_clamp_percent(b);
    ctx->state.rgb_brightness = app_clamp_percent(brightness);
    xSemaphoreGive(ctx->mutex);
    return true;
}

bool app_state_set_curtain_mode(app_context_t *ctx, app_curtain_mode_t mode)
{
    if (!ctx || !ctx->mutex) return false;
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    ctx->state.curtain_mode = mode;
    xSemaphoreGive(ctx->mutex);
    return true;
}

bool app_state_set_curtain_percent(app_context_t *ctx, uint8_t percent)
{
    if (!ctx || !ctx->mutex) return false;
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    ctx->state.curtain_percent = app_clamp_percent(percent);
    xSemaphoreGive(ctx->mutex);
    return true;
}

bool app_state_set_alarm(app_context_t *ctx, bool active)
{
    if (!ctx || !ctx->mutex) return false;
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    ctx->state.alarm_active = active;
    xSemaphoreGive(ctx->mutex);
    return true;
}

bool app_state_set_schedule_record(app_context_t *ctx, uint8_t index, const curtain_schedule_record_t *record)
{
    if (!ctx || !record || !ctx->mutex || index >= APP_CURTAIN_SCHEDULE_RECORDS) return false;
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    ctx->state.schedule[index] = *record;
    ctx->state.schedule[index].percent = app_clamp_percent(record->percent);
    xSemaphoreGive(ctx->mutex);
    return true;
}

bool app_state_get_schedule_record(app_context_t *ctx, uint8_t index, curtain_schedule_record_t *record)
{
    if (!ctx || !record || !ctx->mutex || index >= APP_CURTAIN_SCHEDULE_RECORDS) return false;
    if (xSemaphoreTake(ctx->mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    *record = ctx->state.schedule[index];
    xSemaphoreGive(ctx->mutex);
    return true;
}

bool app_context_send_servo_command(app_context_t *ctx, uint8_t target_percent, bool from_schedule)
{
    if (!ctx || !ctx->servo_queue) return false;
    servo_command_t command = {
        .target_percent = app_clamp_percent(target_percent),
        .from_schedule = from_schedule
    };
    return xQueueSend(ctx->servo_queue, &command, pdMS_TO_TICKS(100)) == pdTRUE;
}
