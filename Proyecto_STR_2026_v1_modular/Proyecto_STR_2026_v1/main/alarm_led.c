#include "alarm_led.h"
#include "app_config.h"

#include "driver/gpio.h"

esp_err_t alarm_led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << APP_GPIO_ALARM_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err == ESP_OK) {
        gpio_set_level(APP_GPIO_ALARM_LED, 0);
    }
    return err;
}

void alarm_led_set(bool on)
{
    gpio_set_level(APP_GPIO_ALARM_LED, on ? 1 : 0);
}
