
idf_component_register(
    SRCS "SoundFX.c" "GateStepper.c" "ClockMode.c" "Wormhole.c" "Settings.c" "base-fw.c" "webserver.c" "GateControl.c" "GPIO.c" "assets/EmbeddedFiles.c"
    INCLUDE_DIRS "."
    REQUIRES SGU drivers nvsjson led_strip esp_rom lwip nvs_flash json esp_http_server esp_wifi app_update fatfs esp_timer esp_hw_support esp_common log)
