#include "Wormhole.h"
#include "GPIO.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "FWConfig.h"
#include "Settings.h"

#define BRIGHTNESS  180

typedef struct
{
    float fOne;
    bool bUp;
} SLedEffect;


static void DoRing0();
static void DoRing1();
static void DoRing2();
static void DoRing3();

static int m_pRing0[] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 15, 16, 17, 18, 19, 47, 48 };
static int m_pRing1[] = { 9, 10, 11, 12, 20, 21, 22, 23, 24, 25, 26, 27, 31, 32, 33, 34, 35, 36 };
static int m_pRing2[] = { 37,38,39,40,28,29,30,42,43  };
static int m_pRing3[] = { 41,44,45,46  };

static SLedEffect m_sLedEffects[FWCONFIG_WORMHOLELEDS_LEDCOUNT];

void WORMHOLE_FullStop()
{
    GPIO_ClearAllPixels();
    GPIO_RefreshPixels();
}

void WORMHOLE_Open(volatile bool* pIsCancelled)
{
    DoRing0();
    DoRing1();
    DoRing2();
    DoRing3();
    vTaskDelay(pdMS_TO_TICKS(200));
    DoRing2();
    DoRing1();
    DoRing0();
}

void WORMHOLE_Run(volatile bool* pIsCancelled, bool bNoTimeLimit)
{
    for(int i = 0; i < FWCONFIG_WORMHOLELEDS_LEDCOUNT; i++)
    {
        m_sLedEffects[i].fOne = (float)(rand() % 100) * 0.01f * 0.15f;
        m_sLedEffects[i].bUp = false;
    }

    // Wait until we get stop command or timeout reached.
    TickType_t xStartTicks = xTaskGetTickCount();

    const uint32_t u32OpenTimeS = SETTINGS_GetValueInt32(SETTINGS_EENTRY_GateOpenedTimeout);

    while(!*pIsCancelled && (bNoTimeLimit || (xTaskGetTickCount() - xStartTicks) < pdMS_TO_TICKS(1000*u32OpenTimeS)))
    {
        for(int i = 0; i < FWCONFIG_WORMHOLELEDS_LEDCOUNT; i++)
        {
            SLedEffect* psLedEffect = &m_sLedEffects[i];

            GPIO_SetPixel(i, psLedEffect->fOne*BRIGHTNESS, psLedEffect->fOne*BRIGHTNESS, BRIGHTNESS);

            const float fInc = 0.008f + ( 0.01f * (rand() % 100) ) * 0.001f;

            if (psLedEffect->bUp)
                psLedEffect->fOne += fInc;
            else
                psLedEffect->fOne -= fInc;

            if (psLedEffect->fOne >= 0.15f)
            {
                psLedEffect->fOne = 0.15f;
                psLedEffect->bUp = false;
            }
            else if (psLedEffect->fOne <= 0.0f)
            {
                psLedEffect->fOne = 0.0f;
                psLedEffect->bUp = true;
            }
        }

        GPIO_RefreshPixels();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void WORMHOLE_Close(volatile bool* pIsCancelled)
{
    DoRing3();
    DoRing2();
    DoRing1();

    // Clear all pixels
    GPIO_ClearAllPixels();
    GPIO_RefreshPixels();
}

static void DoRing0()
{
    float whiteMax = 0.5*BRIGHTNESS;

    for(float brig = 0.0f; brig < 0.50f; brig += 0.04f)
    {
        for(int i = 0; i < sizeof(m_pRing0)/sizeof(int); i++)
            GPIO_SetPixel(m_pRing0[i]-1, whiteMax - BRIGHTNESS * brig, whiteMax - BRIGHTNESS * brig, BRIGHTNESS);

        GPIO_RefreshPixels();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void DoRing1()
{
    float whiteMax = 0.5*BRIGHTNESS;
    for(int i = 0; i < sizeof(m_pRing0)/sizeof(int); i++)
        GPIO_SetPixel(m_pRing0[i]-1, 0, 0, 0);

    for(float brig = 0.0f; brig < 0.50f; brig += 0.04f)
    {
        for(int i = 0; i < sizeof(m_pRing1)/sizeof(int); i++)
            GPIO_SetPixel(m_pRing1[i]-1, whiteMax - BRIGHTNESS * brig, whiteMax - BRIGHTNESS * brig, BRIGHTNESS * brig);
        GPIO_RefreshPixels();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void DoRing2()
{
    float whiteMax = 0.5*BRIGHTNESS;
    for(int i = 0; i < sizeof(m_pRing1)/sizeof(int); i++)
        GPIO_SetPixel(m_pRing1[i]-1, 0, 0, 0);

    for(float brig = 0.0f; brig < 0.50f; brig += 0.03f)
    {
        for(int i = 0; i < sizeof(m_pRing2)/sizeof(int); i++)
            GPIO_SetPixel(m_pRing2[i]-1, whiteMax - BRIGHTNESS * brig, whiteMax - BRIGHTNESS * brig, BRIGHTNESS * brig);

        GPIO_RefreshPixels();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void DoRing3()
{
    float whiteMax = 0.5*BRIGHTNESS;
    for(int i = 0; i < sizeof(m_pRing2)/sizeof(int); i++)
        GPIO_SetPixel(m_pRing1[i]-1, 0, 0, 0);

    for(float brig = 0.0f; brig < 0.50f; brig += 0.04f)
    {
        for(int i = 0; i < sizeof(m_pRing3)/sizeof(int); i++)
            GPIO_SetPixel(m_pRing3[i]-1, whiteMax - BRIGHTNESS * brig, whiteMax - BRIGHTNESS * brig, BRIGHTNESS * brig);

        GPIO_RefreshPixels();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
