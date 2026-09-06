#ifndef ALARM_LED_H
#define ALARM_LED_H

#include <stdbool.h>
#include "esp_err.h"

esp_err_t alarm_led_init(void);
void alarm_led_set(bool on);

#endif // ALARM_LED_H
