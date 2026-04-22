idf_component_register(SRCS "library_led_c.c" "ledc_basic_example_main.c" "library_led_c.h"
                    PRIV_REQUIRES esp_driver_ledc esp_driver_gpio
                    INCLUDE_DIRS ".")
