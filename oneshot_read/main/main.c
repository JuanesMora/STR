#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

#include "library_led_c.h"

// ===== BOTÓN =====
#define BTN_GPIO GPIO_NUM_4

// ===== LED RGB 1 (CONTROL POR POT) =====
#define RED_GPIO    7
#define GREEN_GPIO  18
#define BLUE_GPIO   19

// ===== LED RGB 2 (TEMPERATURA) =====
#define RED2_GPIO    8
#define GREEN2_GPIO  9
#define BLUE2_GPIO   10

// ===== ADC =====
#define ADC_CHANNEL_POT   ADC_CHANNEL_2
#define ADC_CHANNEL_TEMP  ADC_CHANNEL_3

// ===== TERMISTOR =====
#define R_FIXED 10000.0
#define BETA    3950.0
#define T0      298.15   // 25°C en Kelvin
#define R0      10000.0

// ===== ESTADOS =====
typedef enum {
    CONFIG_RED = 0,
    CONFIG_BLUE,
    CONFIG_GREEN,
    SHOW_COLOR
} state_t;

static state_t state = CONFIG_RED;

// ===== VARIABLES =====
static uint8_t red = 0;
static uint8_t green = 0;
static uint8_t blue = 0;

// ===== FUNCIÓN PARA NOMBRE DEL ESTADO =====
static const char* state_to_string(state_t s)
{
    switch (s) {
        case CONFIG_RED:   return "CONFIG_RED";
        case CONFIG_BLUE:  return "CONFIG_BLUE";
        case CONFIG_GREEN: return "CONFIG_GREEN";
        case SHOW_COLOR:   return "SHOW_COLOR";
        default:           return "UNKNOWN";
    }
}

// ===== CONFIG BOTÓN =====
static void config_button(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BTN_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io_conf);
}

// ===== DETECCIÓN DE FLANCO =====
static bool button_pressed(void)
{
    if (gpio_get_level(BTN_GPIO) == 0) {
        while (gpio_get_level(BTN_GPIO) == 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
        return true;
    }
    return false;
}

// ===== CONVERSIÓN A TEMPERATURA =====
static float calcular_temperatura(int adc_raw)
{
    float v = (float)adc_raw / 4095.0;
    float r_ntc = R_FIXED * (v / (1.0 - v));

    float tempK = 1.0 / ( (1.0/T0) + (1.0/BETA)*log(r_ntc/R0) );
    return tempK - 273.15;
}

void app_main(void)
{
    // ===== LED 1 =====
    led_rgb_t led = {
        .led_red = {.gpio_num = RED_GPIO, .channel = LEDC_CHANNEL_0},
        .led_green = {.gpio_num = GREEN_GPIO, .channel = LEDC_CHANNEL_1},
        .led_blue = {.gpio_num = BLUE_GPIO, .channel = LEDC_CHANNEL_2},
        .timer = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .frequency = 4000,
        .speed_mode = LEDC_LOW_SPEED_MODE
    };

    // ===== LED 2 =====
    led_rgb_t led_temp = {
        .led_red = {.gpio_num = RED2_GPIO, .channel = LEDC_CHANNEL_3},
        .led_green = {.gpio_num = GREEN2_GPIO, .channel = LEDC_CHANNEL_4},
        .led_blue = {.gpio_num = BLUE2_GPIO, .channel = LEDC_CHANNEL_5},
        .timer = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .frequency = 4000,
        .speed_mode = LEDC_LOW_SPEED_MODE
    };

    config_led_rgb(&led);
    config_led_rgb(&led_temp);
    config_button();

    // ===== ADC =====
    adc_oneshot_unit_handle_t adc1_handle;

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_POT, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_TEMP, &config));

    printf("Sistema iniciado\n");
    printf("Estado inicial: %s\n\n", state_to_string(state));

    while (1)
    {
        // ===== POT =====
        int adc_raw = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_POT, &adc_raw));
        uint8_t percent = (adc_raw * 100) / 4095;

        // ===== TEMP =====
        int adc_temp = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_TEMP, &adc_temp));
        float temperatura = calcular_temperatura(adc_temp);

        // ===== CAMBIO DE ESTADO =====
        if (button_pressed()) {
            state++;
            if (state > SHOW_COLOR) state = CONFIG_RED;

            printf("\n=== CAMBIO DE ESTADO ===\n");
            printf("Nuevo estado: %s\n", state_to_string(state));
            printf("========================\n\n");
        }

        // ===== MAQUINA DE ESTADOS =====
        const char* estado_str = state_to_string(state);
        switch (state)
        {
            case CONFIG_RED:
                red = percent;
                set_led_rgb_percentage_given_values(&led, red, 0, 0);
                //printf("[CONFIG_RED] %d%%\n", red);
                break;

            case CONFIG_BLUE:
                blue = percent;
                set_led_rgb_percentage_given_values(&led, 0, 0, blue);
                //printf("[CONFIG_BLUE] %d%%\n", blue);
                break;

            case CONFIG_GREEN:
                green = percent;
                set_led_rgb_percentage_given_values(&led, 0, green, 0);
                //printf("[CONFIG_GREEN] %d%%\n", green);
                break;

            case SHOW_COLOR:
                set_led_rgb_percentage_given_values(&led, red, green, blue);
                printf("[FINAL] R:%d G:%d B:%d\n", red, green, blue);
                break;
        }

        // ===== CONTROL POR TEMPERATURA =====
        if (temperatura < 25.0) {
            set_led_rgb_percentage_given_values(&led_temp, 0, 0, 100);
        } else if (temperatura < 35.0) {
            set_led_rgb_percentage_given_values(&led_temp, 0, 100, 0);
        } else {
            set_led_rgb_percentage_given_values(&led_temp, 100, 0, 0);
        }

        printf("\rEstado: %-12s | R:%3d%% G:%3d%% B:%3d%% | Temp: %6.2f C",
        estado_str, red, green, blue, temperatura);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}