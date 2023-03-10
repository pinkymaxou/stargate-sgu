#include "RingControl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static void RingControlTask( void *pvParameters );

static TaskHandle_t m_sRingControlHandle;

void RINGCONTROL_Init()
{

}

void RINGCONTROL_Start()
{
	if (xTaskCreatePinnedToCore(RingControlTask, "ringcontrol", FWCONFIG_RINGCONTROL_STACKSIZE, (void*)NULL, FWCONFIG_RINGCONTROL_PRIORITY_DEFAULT, &m_sRingControlHandle, FWCONFIG_RINGCONTROL_COREID) != pdPASS )
	{
		ESP_ERROR_CHECK(ESP_FAIL);
	}
}

static void RingControlTask( void *pvParameters )
{
    while(1)
    {
        

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}