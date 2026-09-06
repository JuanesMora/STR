#ifndef APP_STORAGE_H
#define APP_STORAGE_H

#include "esp_err.h"
#include "app_state.h"

esp_err_t app_storage_load_schedule(app_context_t *ctx);
esp_err_t app_storage_save_schedule_record(uint8_t index, const curtain_schedule_record_t *record);

#endif // APP_STORAGE_H
