#include <stdio.h>
#include "GateStepper.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "test-stepper"

void app_main(void)
{
    GATESTEPPER_Init();

    while(1)
    {
        ESP_LOGI(TAG, "Move clockwise");
        GATESTEPPER_MoveTo(7333);
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "Move counter clockwise");
        GATESTEPPER_MoveTo(-7333);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
