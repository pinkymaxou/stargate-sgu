#include "Wormhole.h"
#include "GPIO.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "FWConfig.h"
#include "Settings.h"

typedef struct
{
    float fOne;
    bool bUp;
} SLedEffect;

static void DoRing0();
static void DoRing1();
static void DoRing2();
static void DoRing3();

static int GetRing(int zeroBasedIndex);

// 0 = Exterior <====> 3 = interior
// One based, but I should have made it 0 based like a respectable programmer.
static const int m_pRing0[] = { 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 15, 16, 17, 18, 19, 47, 48 };
static const int m_pRing1[] = { 9, 10, 11, 12, 20, 21, 22, 23, 24, 25, 26, 27, 31, 32, 33, 34, 35, 36 };
static const int m_pRing2[] = { 37, 38, 39, 40, 28, 29, 30, 42, 43 };
static const int m_pRing3[] = { 41, 44, 45, 46  };

#define MIN(X,Y) (((X) < (Y)) ? (X) : (Y))

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
    const uint32_t u32MaxBrightness = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WormholeMaxBrightness);

    const float minF = 0.10f;
    const float maxF = 0.25f;

    for(int i = 0; i < FWCONFIG_WORMHOLELEDS_LEDCOUNT; i++)
    {
        m_sLedEffects[i].fOne = (float)(esp_random() % 100) * 0.01f * maxF;
        m_sLedEffects[i].bUp = false;
    }

    // Wait until we get stop command or timeout reached.
    TickType_t xStartTicks = xTaskGetTickCount();

    const uint32_t u32OpenTimeS = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_GateOpenedTimeout);

    while(!*pIsCancelled && (bNoTimeLimit || (xTaskGetTickCount() - xStartTicks) < pdMS_TO_TICKS(1000*u32OpenTimeS)))
    {
        for(int i = 0; i < FWCONFIG_WORMHOLELEDS_LEDCOUNT; i++)
        {
            SLedEffect* psLedEffect = &m_sLedEffects[i];

            const int d = GetRing(i);
            float fFactor = 1.0f;
            switch(d)
            {
                case 3:
                    fFactor = 2.5f;
                    break;
                case 2:
                    fFactor = 1.5f;
                    break;
                case 1:
                    fFactor = 0.6f;
                    break;
                case 0:
                    fFactor = 0.25f;
                    break;
            }

            GPIO_SetPixel(i, MIN(psLedEffect->fOne*u32MaxBrightness * fFactor, 255), MIN(psLedEffect->fOne*u32MaxBrightness * fFactor, 255), MIN(psLedEffect->fOne*u32MaxBrightness * fFactor, 255));

            const float fInc = 0.005f + ( 0.01f * (esp_random() % 100) ) * 0.001f;

            if (psLedEffect->bUp)
                psLedEffect->fOne += fInc;
            else
                psLedEffect->fOne -= fInc;

            if (psLedEffect->fOne >= maxF)
            {
                psLedEffect->fOne = maxF;
                psLedEffect->bUp = false;
            }
            else if (psLedEffect->fOne <= minF)
            {
                psLedEffect->fOne = minF;
                psLedEffect->bUp = true;
            }
        }

        GPIO_RefreshPixels();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static int GetRing(int zeroBasedIndex)
{
    for(int j = 0; j < sizeof(m_pRing0)/sizeof(int); j++)
        if (m_pRing0[j]-1 == zeroBasedIndex)
            return 0;

    for(int j = 0; j < sizeof(m_pRing1)/sizeof(int); j++)
        if (m_pRing1[j]-1 == zeroBasedIndex)
            return 1;

    for(int j = 0; j < sizeof(m_pRing2)/sizeof(int); j++)
        if (m_pRing2[j]-1 == zeroBasedIndex)
            return 2;

    for(int j = 0; j < sizeof(m_pRing3)/sizeof(int); j++)
        if (m_pRing3[j]-1 == zeroBasedIndex)
            return 3;

    return 0;
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
    const uint32_t u32MaxBrightness = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WormholeMaxBrightness);

    float whiteMax = 0.5*u32MaxBrightness;

    for(float brig = 0.0f; brig < 0.50f; brig += 0.04f)
    {
        for(int i = 0; i < sizeof(m_pRing0)/sizeof(int); i++)
            GPIO_SetPixel(m_pRing0[i]-1, whiteMax - u32MaxBrightness * brig, whiteMax - u32MaxBrightness * brig, whiteMax - u32MaxBrightness * brig);

        GPIO_RefreshPixels();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void DoRing1()
{
    const uint32_t u32MaxBrightness = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WormholeMaxBrightness);

    float whiteMax = 0.5*u32MaxBrightness;
    for(int i = 0; i < sizeof(m_pRing0)/sizeof(int); i++)
        GPIO_SetPixel(m_pRing0[i]-1, 0, 0, 0);

    for(float brig = 0.0f; brig < 0.50f; brig += 0.04f)
    {
        for(int i = 0; i < sizeof(m_pRing1)/sizeof(int); i++)
            GPIO_SetPixel(m_pRing1[i]-1, whiteMax - u32MaxBrightness * brig, whiteMax - u32MaxBrightness * brig, whiteMax - u32MaxBrightness * brig);
        GPIO_RefreshPixels();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void DoRing2()
{
    const uint32_t u32MaxBrightness = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WormholeMaxBrightness);

    float whiteMax = 0.5*u32MaxBrightness;
    for(int i = 0; i < sizeof(m_pRing1)/sizeof(int); i++)
        GPIO_SetPixel(m_pRing1[i]-1, 0, 0, 0);

    for(float brig = 0.0f; brig < 0.50f; brig += 0.03f)
    {
        for(int i = 0; i < sizeof(m_pRing2)/sizeof(int); i++)
            GPIO_SetPixel(m_pRing2[i]-1, whiteMax - u32MaxBrightness * brig, whiteMax - u32MaxBrightness * brig, whiteMax - u32MaxBrightness * brig);

        GPIO_RefreshPixels();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void DoRing3()
{
    const uint32_t u32MaxBrightness = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WormholeMaxBrightness);

    float whiteMax = 0.5*u32MaxBrightness;
    for(int i = 0; i < sizeof(m_pRing2)/sizeof(int); i++)
        GPIO_SetPixel(m_pRing1[i]-1, 0, 0, 0);

    for(float brig = 0.0f; brig < 0.50f; brig += 0.04f)
    {
        for(int i = 0; i < sizeof(m_pRing3)/sizeof(int); i++)
            GPIO_SetPixel(m_pRing3[i]-1, whiteMax - u32MaxBrightness * brig, whiteMax - u32MaxBrightness * brig, whiteMax - u32MaxBrightness * brig);

        GPIO_RefreshPixels();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
