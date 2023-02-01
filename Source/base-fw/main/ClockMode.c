#include <time.h>
#include <math.h>
#include "ClockMode.h"
#include "GPIO.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "FWConfig.h"
#include "Settings.h"

#define TAG "ClockMode"

static const int m_pSeconds[] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 15, 16, 17, 18, 19 }; // , 47, 48
#define SECONDS_COUNT (sizeof(m_pSeconds)/sizeof(m_pSeconds[0]))

static const int m_pHours[] = { 35, 36, 20, 21, 22, 23 };
#define HOURS_COUNT (sizeof(m_pHours)/sizeof(m_pHours[0]))

static const int m_pMinutes[] = { 31, 12, 11, 10, 9, 27 };
#define MINUTES_COUNT (sizeof(m_pMinutes)/sizeof(m_pMinutes[0]))


static const int m_pCenters[] = { 46, 45, 44, 41 };
#define CENTERS_COUNT (sizeof(m_pCenters)/sizeof(m_pCenters[0]))

void CLOCKMODE_Run(volatile bool* pIsCancelled)
{
    const uint32_t u32MaxBrightness = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WormholeMaxBrightness);

    GPIO_ClearAllPixels();
    GPIO_RefreshPixels();

    while(!*pIsCancelled)
    {
        time_t now = 0;
        struct tm timeinfo = { 0 };
        time(&now);
        localtime_r(&now, &timeinfo);

        // Hours
        for(int i = 0; i < HOURS_COUNT; i++)
        {
            int trueIndex = m_pHours[i] - 1;

            if (((int)timeinfo.tm_hour & (1 << i)))
                GPIO_SetPixel(trueIndex, 0, u32MaxBrightness, 0);
            else
                GPIO_SetPixel(trueIndex, 40, 40, 40);
        }

        // Minutes
        for(int i = 0; i < MINUTES_COUNT; i++)
        {
            int trueIndex = m_pMinutes[i] - 1;

            if ((int)timeinfo.tm_min & (1 << i))
                GPIO_SetPixel(trueIndex, 0, u32MaxBrightness, 0);
            else
                GPIO_SetPixel(trueIndex, 40, 40, 40);
        }

        // Seconds
        const float fltSeconds = ((float)timeinfo.tm_sec / 60.0f) * SECONDS_COUNT;

        for(int i = 0; i < SECONDS_COUNT; i++)
        {
            int trueIndex = m_pSeconds[i] - 1;

            if (i < (int)fltSeconds)
                GPIO_SetPixel(trueIndex, u32MaxBrightness, 0, u32MaxBrightness);
            else
                GPIO_SetPixel(trueIndex, 10, 10, 10);
        }

        for(int i = 0; i < CENTERS_COUNT; i++)
        {
            int trueIndex = m_pCenters[i] - 1;

            if (timeinfo.tm_hour >= 12)
                GPIO_SetPixel(trueIndex, u32MaxBrightness, u32MaxBrightness, u32MaxBrightness);
            else
                GPIO_SetPixel(trueIndex, u32MaxBrightness, u32MaxBrightness, 0);
        }

        GPIO_RefreshPixels();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    GPIO_ClearAllPixels();
    GPIO_RefreshPixels();
}