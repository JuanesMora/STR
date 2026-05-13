#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

#include "library_led_c.h"

// ===== BOTÓN DEL MODO POT =====
#define BTN_GPIO GPIO_NUM_4

// ===== LED RGB 1 (CONTROL POR POT) =====
#define RED_GPIO    GPIO_NUM_7
#define GREEN_GPIO  GPIO_NUM_18
#define BLUE_GPIO   GPIO_NUM_19

// ===== LED RGB 2 (TEMPERATURA) =====
#define RED2_GPIO    GPIO_NUM_8
#define GREEN2_GPIO  GPIO_NUM_9
#define BLUE2_GPIO   GPIO_NUM_10

// ===== ADC =====
#define ADC_CHANNEL_POT   ADC_CHANNEL_2
#define ADC_CHANNEL_TEMP  ADC_CHANNEL_3

// ===== TERMISTOR =====
#define R_FIXED 10000.0f
#define BETA    3950.0f
#define T0      298.15f   // 25°C en Kelvin
#define R0      10000.0f

// ===== MODOS GENERALES =====
typedef enum {
    MODE_UART_CONFIG = 0,
    MODE_TEMP,
    MODE_POT
} system_mode_t;

// ===== ESTADOS DEL MODO POT =====
typedef enum {
    CONFIG_RED = 0,
    CONFIG_BLUE,
    CONFIG_GREEN,
    SHOW_COLOR
} pot_state_t;

// ===== CONFIG DE CADA COLOR EN MODO TERMISTOR =====
typedef struct {
    float temp_min;
    float temp_max;
    uint8_t intensity;
} color_temp_cfg_t;

// ===== VARIABLES GLOBALES =====
static volatile system_mode_t current_mode = MODE_UART_CONFIG;
static volatile bool uart_busy = false;

static pot_state_t pot_state = CONFIG_RED;

static uint8_t red = 0;
static uint8_t green = 0;
static uint8_t blue = 0;

static color_temp_cfg_t red_cfg   = {32.0f, 40.0f, 100};
static color_temp_cfg_t green_cfg = {25.0f, 31.0f, 100};
static color_temp_cfg_t blue_cfg  = {18.0f, 24.0f, 100};

static adc_oneshot_unit_handle_t adc1_handle;

// ===== LED RGB 1 =====
static led_rgb_t led = {
    .led_red = {.gpio_num = RED_GPIO, .channel = LEDC_CHANNEL_0, .duty = 0},
    .led_green = {.gpio_num = GREEN_GPIO, .channel = LEDC_CHANNEL_1, .duty = 0},
    .led_blue = {.gpio_num = BLUE_GPIO, .channel = LEDC_CHANNEL_2, .duty = 0},
    .timer = LEDC_TIMER_0,
    .duty_resolution = LEDC_TIMER_13_BIT,
    .frequency = 4000,
    .speed_mode = LEDC_LOW_SPEED_MODE
};

// ===== LED RGB 2 =====
static led_rgb_t led_temp = {
    .led_red = {.gpio_num = RED2_GPIO, .channel = LEDC_CHANNEL_3, .duty = 0},
    .led_green = {.gpio_num = GREEN2_GPIO, .channel = LEDC_CHANNEL_4, .duty = 0},
    .led_blue = {.gpio_num = BLUE2_GPIO, .channel = LEDC_CHANNEL_5, .duty = 0},
    .timer = LEDC_TIMER_0,
    .duty_resolution = LEDC_TIMER_13_BIT,
    .frequency = 4000,
    .speed_mode = LEDC_LOW_SPEED_MODE
};

static const char* mode_to_string(system_mode_t mode)
{
    switch (mode) {
        case MODE_UART_CONFIG: return "MODE_UART_CONFIG";
        case MODE_TEMP:        return "MODE_TEMP";
        case MODE_POT:         return "MODE_POT";
        default:               return "UNKNOWN";
    }
}

static const char* pot_state_to_string(pot_state_t s)
{
    switch (s) {
        case CONFIG_RED:   return "CONFIG_RED";
        case CONFIG_BLUE:  return "CONFIG_BLUE";
        case CONFIG_GREEN: return "CONFIG_GREEN";
        case SHOW_COLOR:   return "SHOW_COLOR";
        default:           return "UNKNOWN";
    }
}

static void config_button(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BTN_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

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

static void config_adc(void)
{
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
}

static float calcular_temperatura(int adc_raw)
{
    if (adc_raw <= 0) {
        adc_raw = 1;
    }
    if (adc_raw >= 4095) {
        adc_raw = 4094;
    }

    float v = (float)adc_raw / 4095.0f;
    float r_ntc = R_FIXED * (v / (1.0f - v));

    float tempK = 1.0f / ((1.0f / T0) + (1.0f / BETA) * logf(r_ntc / R0));
    return tempK - 273.15f;
}

static void turn_off_led(led_rgb_t *rgb)
{
    set_led_rgb_percentage_given_values(rgb, 0, 0, 0);
}

static void strip_newline(char *text)
{
    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) {
        text[len - 1] = '\0';
        len--;
    }
}

static void to_uppercase(char *text)
{
    for (int i = 0; text[i] != '\0'; i++) {
        text[i] = (char)toupper((unsigned char)text[i]);
    }
}

// ===== LECTURA UART CORREGIDA =====
static void read_line_uart(char *buffer, size_t size)
{
    int index = 0;
    int c = 0;

    uart_busy = true;

    while (1) {
        c = getchar();

        if (c == EOF) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (c == '\r' || c == '\n') {
            buffer[index] = '\0';
            printf("\n");
            fflush(stdout);
            uart_busy = false;
            return;
        }

        if ((c == '\b' || c == 127) && index > 0) {
            index--;
            printf("\b \b");
            fflush(stdout);
            continue;
        }

        if (isprint(c) && index < (int)(size - 1)) {
            buffer[index++] = (char)c;
            putchar(c);
            fflush(stdout);
        }
    }
}

static float read_float_uart(const char *prompt)
{
    char buffer[64];
    float value = 0.0f;

    while (1) {
        printf("%s", prompt);
        fflush(stdout);

        read_line_uart(buffer, sizeof(buffer));

        if (sscanf(buffer, "%f", &value) == 1) {
            return value;
        }

        printf("Valor invalido. Intente nuevamente.\n");
    }
}

static int read_int_uart(const char *prompt, int min_value, int max_value)
{
    char buffer[64];
    int value = 0;

    while (1) {
        printf("%s", prompt);
        fflush(stdout);

        read_line_uart(buffer, sizeof(buffer));

        if (sscanf(buffer, "%d", &value) == 1) {
            if (value >= min_value && value <= max_value) {
                return value;
            }
        }

        printf("Valor invalido. Debe estar entre %d y %d.\n", min_value, max_value);
    }
}

static void print_temp_config(void)
{
    printf("\n===== CONFIGURACION ACTUAL DEL MODO TERMISTOR =====\n");
    printf("RED   -> min: %.2f  max: %.2f  intensidad: %d%%\n",
           red_cfg.temp_min, red_cfg.temp_max, red_cfg.intensity);
    printf("GREEN -> min: %.2f  max: %.2f  intensidad: %d%%\n",
           green_cfg.temp_min, green_cfg.temp_max, green_cfg.intensity);
    printf("BLUE  -> min: %.2f  max: %.2f  intensidad: %d%%\n",
           blue_cfg.temp_min, blue_cfg.temp_max, blue_cfg.intensity);
    printf("===============================================\n\n");
}

static void configure_temp_mode_uart(void)
{
    current_mode = MODE_UART_CONFIG;
    turn_off_led(&led);
    turn_off_led(&led_temp);

    printf("\n===== FASE 1: RANGOS DE TEMPERATURA =====\n");

    do {
        red_cfg.temp_min = read_float_uart("Defina el min para el color RED: ");
        red_cfg.temp_max = read_float_uart("Defina el max para el color RED: ");
        if (red_cfg.temp_max < red_cfg.temp_min) {
            printf("Error: el maximo no puede ser menor que el minimo.\n");
        }
    } while (red_cfg.temp_max < red_cfg.temp_min);

    do {
        green_cfg.temp_min = read_float_uart("Defina el min para el color GREEN: ");
        green_cfg.temp_max = read_float_uart("Defina el max para el color GREEN: ");
        if (green_cfg.temp_max < green_cfg.temp_min) {
            printf("Error: el maximo no puede ser menor que el minimo.\n");
        }
    } while (green_cfg.temp_max < green_cfg.temp_min);

    do {
        blue_cfg.temp_min = read_float_uart("Defina el min para el color BLUE: ");
        blue_cfg.temp_max = read_float_uart("Defina el max para el color BLUE: ");
        if (blue_cfg.temp_max < blue_cfg.temp_min) {
            printf("Error: el maximo no puede ser menor que el minimo.\n");
        }
    } while (blue_cfg.temp_max < blue_cfg.temp_min);

    printf("\n===== FASE 2: INTENSIDAD DE CADA COLOR =====\n");

    red_cfg.intensity   = (uint8_t)read_int_uart("Defina intensidad RED (0-100): ", 0, 100);
    green_cfg.intensity = (uint8_t)read_int_uart("Defina intensidad GREEN (0-100): ", 0, 100);
    blue_cfg.intensity  = (uint8_t)read_int_uart("Defina intensidad BLUE (0-100): ", 0, 100);

    print_temp_config();

    printf("Modo termistor configurado.\n");
    printf("Comandos disponibles por UART:\n");
    printf("  POT    -> pasar al modo potenciometro\n");
    printf("  TEMP   -> volver al modo termistor\n");
    printf("  SET    -> reconfigurar rangos e intensidades\n");
    printf("  STATUS -> mostrar configuracion actual\n\n");

    current_mode = MODE_TEMP;
}

static void apply_temperature_rgb(float temperatura, int *out_r, int *out_g, int *out_b)
{
    *out_r = 0;
    *out_g = 0;
    *out_b = 0;

    if (temperatura >= red_cfg.temp_min && temperatura <= red_cfg.temp_max) {
        *out_r = red_cfg.intensity;
    }
    if (temperatura >= green_cfg.temp_min && temperatura <= green_cfg.temp_max) {
        *out_g = green_cfg.intensity;
    }
    if (temperatura >= blue_cfg.temp_min && temperatura <= blue_cfg.temp_max) {
        *out_b = blue_cfg.intensity;
    }

    set_led_rgb_percentage_given_values(&led_temp, *out_r, *out_g, *out_b);
}

static void uart_task(void *pvParameter)
{
    char command[64];

    while (1) {
        uart_busy = true;
        printf("\nComando UART (POT/TEMP/SET/STATUS): ");
        fflush(stdout);

        read_line_uart(command, sizeof(command));
        strip_newline(command);
        to_uppercase(command);

        if (strcmp(command, "POT") == 0) {
            current_mode = MODE_POT;
            turn_off_led(&led_temp);
            printf("Cambio realizado -> %s\n", mode_to_string(current_mode));
        }
        else if (strcmp(command, "TEMP") == 0) {
            current_mode = MODE_TEMP;
            turn_off_led(&led);
            printf("Cambio realizado -> %s\n", mode_to_string(current_mode));
        }
        else if (strcmp(command, "SET") == 0) {
            configure_temp_mode_uart();
        }
        else if (strcmp(command, "STATUS") == 0) {
            printf("Modo actual: %s\n", mode_to_string(current_mode));
            print_temp_config();
            printf("Modo POT -> Estado: %s | R:%d%% G:%d%% B:%d%%\n",
                   pot_state_to_string(pot_state), red, green, blue);
        }
        else {
            printf("Comando no valido.\n");
        }

        uart_busy = false;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void control_task(void *pvParameter)
{
    int print_divider = 0;

    while (1) {
        int adc_raw_pot = 0;
        int adc_raw_temp = 0;

        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_POT, &adc_raw_pot));
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_TEMP, &adc_raw_temp));

        uint8_t percent = (adc_raw_pot * 100) / 4095;
        float temperatura = calcular_temperatura(adc_raw_temp);

        if (current_mode == MODE_UART_CONFIG) {
            turn_off_led(&led);
            turn_off_led(&led_temp);
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        if (current_mode == MODE_TEMP) {
            int temp_r = 0;
            int temp_g = 0;
            int temp_b = 0;

            turn_off_led(&led);
            apply_temperature_rgb(temperatura, &temp_r, &temp_g, &temp_b);

            print_divider++;
            if (print_divider >= 4) {
                print_divider = 0;
                printf("[TEMP] Temp: %6.2f C | RGB2 -> R:%3d%% G:%3d%% B:%3d%%\n",
                       temperatura, temp_r, temp_g, temp_b);
            }
        }
        else if (current_mode == MODE_POT) {
            turn_off_led(&led_temp);

            if (button_pressed()) {
                pot_state++;
                if (pot_state > SHOW_COLOR) {
                    pot_state = CONFIG_RED;
                }

                printf("\n=== CAMBIO DE ESTADO POT ===\n");
                printf("Nuevo estado: %s\n", pot_state_to_string(pot_state));
                printf("============================\n");
            }

            switch (pot_state) {
                case CONFIG_RED:
                    red = percent;
                    set_led_rgb_percentage_given_values(&led, red, 0, 0);
                    break;

                case CONFIG_BLUE:
                    blue = percent;
                    set_led_rgb_percentage_given_values(&led, 0, 0, blue);
                    break;

                case CONFIG_GREEN:
                    green = percent;
                    set_led_rgb_percentage_given_values(&led, 0, green, 0);
                    break;

                case SHOW_COLOR:
                    set_led_rgb_percentage_given_values(&led, red, green, blue);
                    break;
            }

            print_divider++;
            if (print_divider >= 4) {
                print_divider = 0;
                printf("[POT ] Estado: %-12s | POT:%3d%% | RGB1 -> R:%3d%% G:%3d%% B:%3d%%\n",
                       pot_state_to_string(pot_state), percent, red, green, blue);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void app_main(void)
{
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    config_led_rgb(&led);
    config_led_rgb(&led_temp);
    config_button();
    config_adc();

    turn_off_led(&led);
    turn_off_led(&led_temp);

    printf("Sistema iniciado.\n");
    printf("Primero se configurara el modo del termistor por UART.\n");

    configure_temp_mode_uart();

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 5, NULL);
    xTaskCreate(control_task, "control_task", 4096, NULL, 5, NULL);
}