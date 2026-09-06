#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/ledc.h"

/*
 * CONFIGURACION CENTRAL DEL PROYECTO
 * Si te equivocas en un pin, valor del NTC, Beta, frecuencia o tiempo de servo,
 * normalmente solo debes cambiarlo aqui.
 */

// =========================
// ESP32-C6 / ADC / NTC
// =========================
#define APP_NTC_ADC_UNIT                    ADC_UNIT_1
#define APP_NTC_ADC_CHANNEL                 ADC_CHANNEL_3      // ADC_CHANNEL_3 suele corresponder a GPIO3 en ESP32-C6.
#define APP_NTC_ADC_ATTEN                   ADC_ATTEN_DB_12
#define APP_NTC_ADC_BITWIDTH                ADC_BITWIDTH_DEFAULT

#define APP_NTC_VCC                         3.3f
#define APP_NTC_R0_OHM                      14000.0f            // CAMBIA AQUI si tu NTC no es de 14 kOhm.
#define APP_NTC_BETA                        3950.0f             // CAMBIA AQUI si tu beta no es 3950.
#define APP_NTC_T0_K                        298.15f             // 25 grados Celsius en Kelvin.
#define APP_NTC_FIXED_RESISTOR_OHM          10000.0f            // CAMBIA AQUI si la resistencia fija fisica no es de 10 kOhm.

// 1: divisor VCC -> resistencia fija -> ADC -> NTC -> GND.
// 0: divisor VCC -> NTC -> ADC -> resistencia fija -> GND.
#define APP_NTC_TO_GND                      1

// =========================
// Pines de actuadores
// =========================
#define APP_GPIO_ALARM_LED                  GPIO_NUM_2
#define APP_GPIO_FAN_PWM                    GPIO_NUM_4
#define APP_GPIO_SERVO_PWM                  GPIO_NUM_5

// LED RGB de catodo comun: mas duty = mas brillo.
// Si luego usas anodo comun, cambia APP_RGB_COMMON_CATHODE a 0.
#define APP_RGB_COMMON_CATHODE              1
#define APP_GPIO_RGB_RED                    GPIO_NUM_8
#define APP_GPIO_RGB_GREEN                  GPIO_NUM_9
#define APP_GPIO_RGB_BLUE                   GPIO_NUM_10

// =========================
// LEDC PWM - ESP32-C6
// =========================
#define APP_LEDC_MODE                       LEDC_LOW_SPEED_MODE  // ESP32-C6: usar low speed.

#define APP_RGB_LEDC_TIMER                  LEDC_TIMER_0
#define APP_RGB_LEDC_FREQ_HZ                4000
#define APP_RGB_LEDC_RES                    LEDC_TIMER_13_BIT
#define APP_RGB_CH_RED                      LEDC_CHANNEL_0
#define APP_RGB_CH_GREEN                    LEDC_CHANNEL_1
#define APP_RGB_CH_BLUE                     LEDC_CHANNEL_2

#define APP_FAN_LEDC_TIMER                  LEDC_TIMER_1
#define APP_FAN_LEDC_FREQ_HZ                25000               // Frecuencia tipica para ventilador DC por PWM.
#define APP_FAN_LEDC_RES                    LEDC_TIMER_13_BIT
#define APP_FAN_LEDC_CHANNEL                LEDC_CHANNEL_3

#define APP_SERVO_LEDC_TIMER                LEDC_TIMER_2
#define APP_SERVO_LEDC_FREQ_HZ              50                  // Servo: periodo de 20 ms.
#define APP_SERVO_LEDC_RES                  LEDC_TIMER_14_BIT
#define APP_SERVO_LEDC_CHANNEL              LEDC_CHANNEL_4

// =========================
// Servo continuo SG90 360 grados
// =========================
#define APP_SERVO_STOP_US                   1500                // Punto neutro. Ajusta si el servo se mueve quieto.
#define APP_SERVO_OPEN_US                   1700                // Giro hacia abrir. Ajusta sentido/velocidad.
#define APP_SERVO_CLOSE_US                  1300                // Giro hacia cerrar. Ajusta sentido/velocidad.
#define APP_SERVO_FULL_TRAVEL_MS            2500                // Tiempo estimado de 0% a 100% de cortina.
#define APP_SERVO_SETTLE_MS                 250

// =========================
// Control de temperatura por defecto
// =========================
#define APP_DEFAULT_TEMP_DESIRED_C          25.0f
#define APP_DEFAULT_TEMP_MAX_C              30.0f
#define APP_DEFAULT_FAN_AUTO_MODE           1
#define APP_DEFAULT_FAN_PERCENT             0

// =========================
// RGB por defecto
// =========================
#define APP_DEFAULT_RGB_R                   0
#define APP_DEFAULT_RGB_G                   0
#define APP_DEFAULT_RGB_B                   100
#define APP_DEFAULT_RGB_BRIGHTNESS          60

// =========================
// Cortinas / agenda
// =========================
#define APP_CURTAIN_SCHEDULE_RECORDS        8
#define APP_DEFAULT_CURTAIN_PERCENT         0
#define APP_DEFAULT_CURTAIN_AUTO_MODE       0

// =========================
// Tareas FreeRTOS
// =========================
#define APP_TASK_TEMP_STACK                 4096
#define APP_TASK_FAN_STACK                  3072
#define APP_TASK_RGB_STACK                  3072
#define APP_TASK_SERVO_STACK                4096
#define APP_TASK_ALARM_STACK                2048
#define APP_TASK_SCHEDULE_STACK             4096

#define APP_TASK_TEMP_PRIORITY              5
#define APP_TASK_FAN_PRIORITY               4
#define APP_TASK_RGB_PRIORITY               3
#define APP_TASK_SERVO_PRIORITY             4
#define APP_TASK_ALARM_PRIORITY             3
#define APP_TASK_SCHEDULE_PRIORITY          3

#define APP_TASK_TEMP_PERIOD_MS             1000
#define APP_TASK_FAN_PERIOD_MS              500
#define APP_TASK_RGB_PERIOD_MS              500
#define APP_TASK_ALARM_PERIOD_MS            500
#define APP_TASK_SCHEDULE_PERIOD_MS         10000

#endif // APP_CONFIG_H
