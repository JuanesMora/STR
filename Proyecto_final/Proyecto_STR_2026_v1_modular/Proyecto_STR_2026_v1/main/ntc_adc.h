#ifndef NTC_ADC_H
#define NTC_ADC_H

#include <stdbool.h>
#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"

typedef struct {
    adc_oneshot_unit_handle_t adc_handle;
    adc_cali_handle_t cali_handle;
    bool calibration_enabled;
} ntc_adc_t;

esp_err_t ntc_adc_init(ntc_adc_t *ntc);
esp_err_t ntc_adc_read(ntc_adc_t *ntc, int *raw, float *voltage_v, float *temperature_c);
void ntc_adc_deinit(ntc_adc_t *ntc);

#endif // NTC_ADC_H
