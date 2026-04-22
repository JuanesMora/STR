#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "library_led_c.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY          (4000)

// GPIO del LED RGB externo
#define LED_RED_GPIO            GPIO_NUM_2
#define LED_GREEN_GPIO          GPIO_NUM_3
#define LED_BLUE_GPIO           GPIO_NUM_4

// GPIO de los tres pulsadores
#define BUTTON_RED_GPIO         GPIO_NUM_5
#define BUTTON_GREEN_GPIO       GPIO_NUM_6
#define BUTTON_BLUE_GPIO        GPIO_NUM_7

#define STEP_PERCENT            10

void config_buttons(void)
{
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << BUTTON_RED_GPIO) |
                           (1ULL << BUTTON_GREEN_GPIO) |
                           (1ULL << BUTTON_BLUE_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
}

void app_main(void)
{
    led_rgb_t led_rgb = {
        .led_red = {
            .duty = 0,
            .gpio_num = LED_RED_GPIO,
            .channel = LEDC_CHANNEL_0
        },
        .led_green = {
            .duty = 0,
            .gpio_num = LED_GREEN_GPIO,
            .channel = LEDC_CHANNEL_1
        },
        .led_blue = {
            .duty = 0,
            .gpio_num = LED_BLUE_GPIO,
            .channel = LEDC_CHANNEL_2
        },
        .timer = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .frequency = LEDC_FREQUENCY,
        .speed_mode = LEDC_MODE
    };

    int red_percentage = 0;
    int green_percentage = 0;
    int blue_percentage = 0;

    config_led_rgb(&led_rgb);
    config_buttons();

    set_led_rgb_percentage_given_values(&led_rgb, red_percentage, green_percentage, blue_percentage);

    printf("Estado inicial -> R:%d%% G:%d%% B:%d%%\n",
           red_percentage, green_percentage, blue_percentage);

    while (1) {

        if (gpio_get_level(BUTTON_RED_GPIO) == 0) {
            if (red_percentage < 100) {
                red_percentage += STEP_PERCENT;
            }

            set_led_rgb_percentage_given_values(&led_rgb, red_percentage, green_percentage, blue_percentage);

            printf("Rojo -> R:%d%% G:%d%% B:%d%%\n",
                   red_percentage, green_percentage, blue_percentage);

            while (gpio_get_level(BUTTON_RED_GPIO) == 0) {
                vTaskDelay(20 / portTICK_PERIOD_MS);
            }
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        if (gpio_get_level(BUTTON_GREEN_GPIO) == 0) {
            if (green_percentage < 100) {
                green_percentage += STEP_PERCENT;
            }

            set_led_rgb_percentage_given_values(&led_rgb, red_percentage, green_percentage, blue_percentage);

            printf("Verde -> R:%d%% G:%d%% B:%d%%\n",
                   red_percentage, green_percentage, blue_percentage);

            while (gpio_get_level(BUTTON_GREEN_GPIO) == 0) {
                vTaskDelay(20 / portTICK_PERIOD_MS);
            }
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        if (gpio_get_level(BUTTON_BLUE_GPIO) == 0) {
            if (blue_percentage < 100) {
                blue_percentage += STEP_PERCENT;
            }

            set_led_rgb_percentage_given_values(&led_rgb, red_percentage, green_percentage, blue_percentage);

            printf("Azul -> R:%d%% G:%d%% B:%d%%\n",
                   red_percentage, green_percentage, blue_percentage);

            while (gpio_get_level(BUTTON_BLUE_GPIO) == 0) {
                vTaskDelay(20 / portTICK_PERIOD_MS);
            }
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}