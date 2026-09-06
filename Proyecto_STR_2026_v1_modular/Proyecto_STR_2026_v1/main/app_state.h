#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "app_config.h"

typedef enum {
    APP_FAN_MODE_AUTO = 0,
    APP_FAN_MODE_MANUAL
} app_fan_mode_t;

typedef enum {
    APP_CURTAIN_MODE_MANUAL = 0,
    APP_CURTAIN_MODE_AUTO
} app_curtain_mode_t;

typedef struct {
    bool enabled;
    uint8_t hour;       // 0-23
    uint8_t minute;     // 0-59
    uint8_t percent;    // 0-100
} curtain_schedule_record_t;

typedef struct {
    uint8_t target_percent;
    bool from_schedule;
} servo_command_t;

typedef struct {
    float temperature_c;
    int adc_raw;
    float adc_voltage_v;

    float temp_desired_c;
    float temp_max_c;

    app_fan_mode_t fan_mode;
    uint8_t fan_percent;

    uint8_t rgb_r;
    uint8_t rgb_g;
    uint8_t rgb_b;
    uint8_t rgb_brightness;

    app_curtain_mode_t curtain_mode;
    uint8_t curtain_percent;

    bool alarm_active;

    curtain_schedule_record_t schedule[APP_CURTAIN_SCHEDULE_RECORDS];
} app_state_t;

typedef struct {
    app_state_t state;
    SemaphoreHandle_t mutex;
    QueueHandle_t servo_queue;
} app_context_t;

void app_context_init(app_context_t *ctx);

bool app_state_get_snapshot(app_context_t *ctx, app_state_t *out_state);
bool app_state_set_temperature(app_context_t *ctx, float temperature_c, int adc_raw, float voltage_v);
bool app_state_set_fan_config(app_context_t *ctx, app_fan_mode_t mode, float desired_c, float max_c, uint8_t manual_percent);
bool app_state_set_fan_percent(app_context_t *ctx, uint8_t fan_percent);
bool app_state_set_rgb(app_context_t *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
bool app_state_set_curtain_mode(app_context_t *ctx, app_curtain_mode_t mode);
bool app_state_set_curtain_percent(app_context_t *ctx, uint8_t percent);
bool app_state_set_alarm(app_context_t *ctx, bool active);
bool app_state_set_schedule_record(app_context_t *ctx, uint8_t index, const curtain_schedule_record_t *record);
bool app_state_get_schedule_record(app_context_t *ctx, uint8_t index, curtain_schedule_record_t *record);

bool app_context_send_servo_command(app_context_t *ctx, uint8_t target_percent, bool from_schedule);

uint8_t app_clamp_percent(int value);

#endif // APP_STATE_H
