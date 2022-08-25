#include "ClockMode.h"
#include "GPIO.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "FWConfig.h"
#include "Settings.h"

static const int m_pRing0[] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 15, 16, 17, 18, 19, 47, 48 };
#define RING0_COUNT (sizeof(m_pRing0)/sizeof(m_pRing0[0]))

void CLOCKMODE_Run(volatile bool* pIsCancelled)
{
    GPIO_ClearAllPixels();
    GPIO_RefreshPixels();

    int ix = 0;

    while(!*pIsCancelled)
    {
        for(int i = 0; i < RING0_COUNT; i++)
        {
            if (i == ix)
                GPIO_SetPixel(i, 255, 0, 0);
            else
                GPIO_SetPixel(i, 0, 0, 255);
        }

        ix = (ix + 1) % RING0_COUNT;

        GPIO_RefreshPixels();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    GPIO_ClearAllPixels();
    GPIO_RefreshPixels();
}