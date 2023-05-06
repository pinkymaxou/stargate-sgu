idf_component_register(SRCS "main.c" "webserver.c" "webserver.h" "assets/EmbeddedFiles.c" "gpio.c" "gpio.h"
INCLUDE_DIRS "."
REQUIRES esp_pm driver lwip nvs_flash json esp_http_server esp_wifi app_update esp_hw_support esp_common log esp_app_format led_strip)