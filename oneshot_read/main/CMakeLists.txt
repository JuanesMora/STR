idf_component_register(
    SRCS  "main.c" "library_led_c.c"
    PRIV_REQUIRES driver esp_adc freertos
    INCLUDE_DIRS "."
)