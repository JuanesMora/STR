#include "app_storage.h"
#include "app_config.h"

#include <stdio.h>
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "app_storage";

static void schedule_key(uint8_t index, char *key, size_t key_len)
{
    snprintf(key, key_len, "cur%02u", index);
}

esp_err_t app_storage_save_schedule_record(uint8_t index, const curtain_schedule_record_t *record)
{
    if (!record || index >= APP_CURTAIN_SCHEDULE_RECORDS) return ESP_ERR_INVALID_ARG;

    nvs_handle_t nvs;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;

    char key[8];
    schedule_key(index, key, sizeof(key));

    err = nvs_set_blob(nvs, key, record, sizeof(curtain_schedule_record_t));
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Schedule record %u saved", index);
    }

    return err;
}

esp_err_t app_storage_load_schedule(app_context_t *ctx)
{
    if (!ctx) return ESP_ERR_INVALID_ARG;

    nvs_handle_t nvs;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No NVS schedule found yet");
        return err;
    }

    for (uint8_t i = 0; i < APP_CURTAIN_SCHEDULE_RECORDS; i++) {
        char key[8];
        schedule_key(i, key, sizeof(key));

        curtain_schedule_record_t record;
        size_t required = sizeof(record);
        err = nvs_get_blob(nvs, key, &record, &required);
        if (err == ESP_OK && required == sizeof(record)) {
            if (record.hour > 23 || record.minute > 59 || record.percent > 100) {
                record.enabled = false;
            }
            app_state_set_schedule_record(ctx, i, &record);
            ESP_LOGI(TAG, "Schedule record %u loaded", i);
        }
    }

    nvs_close(nvs);
    return ESP_OK;
}
