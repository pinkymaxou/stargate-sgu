idf_component_register(SRCS "gpio.c" "main.c"
    INCLUDE_DIRS "."
    REQUIRES lwip log esp_common esp_app_format esp_event esp_wifi nvs_flash esp_pm driver SGU app_update led_strip)
