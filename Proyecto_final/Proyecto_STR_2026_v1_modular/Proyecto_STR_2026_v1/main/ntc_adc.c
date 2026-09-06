#include "ntc_adc.h"
#include "app_config.h"

#include <math.h>
#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "ntc_adc";

esp_err_t ntc_adc_init(ntc_adc_t *ntc)
{
    if (!ntc) return ESP_ERR_INVALID_ARG;
    memset(ntc, 0, sizeof(ntc_adc_t));

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = APP_NTC_ADC_UNIT,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&init_config, &ntc->adc_handle), TAG, "adc_oneshot_new_unit failed");

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = APP_NTC_ADC_ATTEN,
        .bitwidth = APP_NTC_ADC_BITWIDTH,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(ntc->adc_handle, APP_NTC_ADC_CHANNEL, &channel_config), TAG, "adc channel config failed");

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = APP_NTC_ADC_UNIT,
        .chan = APP_NTC_ADC_CHANNEL,
        .atten = APP_NTC_ADC_ATTEN,
        .bitwidth = APP_NTC_ADC_BITWIDTH,
    };

    esp_err_t err = adc_cali_create_scheme_curve_fitting(&cali_config, &ntc->cali_handle);
    if (err == ESP_OK) {
        ntc->calibration_enabled = true;
        ESP_LOGI(TAG, "ADC calibration enabled");
    } else {
        ntc->calibration_enabled = false;
        ESP_LOGW(TAG, "ADC calibration not available, using raw estimation");
    }

    return ESP_OK;
}

esp_err_t ntc_adc_read(ntc_adc_t *ntc, int *raw, float *voltage_v, float *temperature_c)
{
    if (!ntc || !raw || !voltage_v || !temperature_c) return ESP_ERR_INVALID_ARG;

    int adc_raw = 0;
    ESP_RETURN_ON_ERROR(adc_oneshot_read(ntc->adc_handle, APP_NTC_ADC_CHANNEL, &adc_raw), TAG, "adc read failed");

    float voltage = 0.0f;
    if (ntc->calibration_enabled) {
        int voltage_mv = 0;
        ESP_RETURN_ON_ERROR(adc_cali_raw_to_voltage(ntc->cali_handle, adc_raw, &voltage_mv), TAG, "adc calibration conversion failed");
        voltage = voltage_mv / 1000.0f;
    } else {
        voltage = ((float)adc_raw / 4095.0f) * APP_NTC_VCC;
    }

    if (voltage >= (APP_NTC_VCC - 0.01f)) voltage = APP_NTC_VCC - 0.01f;
    if (voltage <= 0.01f) voltage = 0.01f;

    float r_ntc;
#if APP_NTC_TO_GND
    // VCC -> resistencia fija -> ADC -> NTC -> GND
    r_ntc = APP_NTC_FIXED_RESISTOR_OHM * (voltage / (APP_NTC_VCC - voltage));
#else
    // VCC -> NTC -> ADC -> resistencia fija -> GND
    r_ntc = APP_NTC_FIXED_RESISTOR_OHM * ((APP_NTC_VCC - voltage) / voltage);
#endif

    if (r_ntc <= 1.0f) r_ntc = 1.0f;

    float temp_k = 1.0f / ((1.0f / APP_NTC_T0_K) + (1.0f / APP_NTC_BETA) * logf(r_ntc / APP_NTC_R0_OHM));

    *raw = adc_raw;
    *voltage_v = voltage;
    *temperature_c = temp_k - 273.15f;

    return ESP_OK;
}

void ntc_adc_deinit(ntc_adc_t *ntc)
{
    if (!ntc) return;
    if (ntc->calibration_enabled && ntc->cali_handle) {
        adc_cali_delete_scheme_curve_fitting(ntc->cali_handle);
    }
    if (ntc->adc_handle) {
        adc_oneshot_del_unit(ntc->adc_handle);
    }
    memset(ntc, 0, sizeof(ntc_adc_t));
}
