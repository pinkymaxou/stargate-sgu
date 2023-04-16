
idf_component_register(
    SRCS "SoundFX.c" "GateStepper.c" "ClockMode.c" "Wormhole.c" "Settings.c" 
    "Main.c" "Main.h"
    "webserver/WebServer.c" "webserver/WebServer.h" "webserver/EmbeddedFiles.c" "webserver/EmbeddedFiles.h"
    "webserver/WebAPI.c" "webserver/WebAPI.h" "webserver/ApiURL.h"
    "GateControl.c" "GPIO.c"
    INCLUDE_DIRS "."
    REQUIRES SGU SSD1306 lwip nvsjson led_strip mdns nvs_flash json esp_http_server esp_wifi app_update fatfs esp_timer esp_hw_support esp_common log)
