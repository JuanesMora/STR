#include <stdlib.h>
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi_app.h"
#include "http_server.h"
#include "app_state.h"
#include "app_tasks.h"
#include "app_storage.h"

static const char *TAG = "main";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Starting STR 2026 - Sistema Automatizado de Control Ambiental");

    app_context_t *app_ctx = calloc(1, sizeof(app_context_t));
    if (app_ctx == NULL) {
        ESP_LOGE(TAG, "No memory for app context");
        abort();
    }

    app_context_init(app_ctx);
    app_storage_load_schedule(app_ctx);

    // El servidor HTTP conserva la OTA del proyecto base, pero ahora tambien conoce el estado del sistema.
    http_server_set_app_context(app_ctx);

    // SNTP se sincroniza cuando el modo STA logra conectarse a internet.
    init_obtain_time();

    // Tareas FreeRTOS de sensores/actuadores.
    ESP_ERROR_CHECK(app_tasks_start(app_ctx));

    // WiFi AP + STA + HTTP server + OTA del proyecto base del profesor.
    wifi_app_start();
}
