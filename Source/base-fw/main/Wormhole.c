#include "Wormhole.h"
#include "GPIO.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_random.h"
#include "FWConfig.h"
#include "Settings.h"
#include "SoundFX.h"
#include "HelperMacro.h"

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

static SLedEffect m_sLedEffects[HWCONFIG_WORMHOLELEDS_LEDCOUNT];
static WORMHOLE_SArg m_sArg = {0};

static bool m_bIsIdlingSoundPlaying = false;
static TickType_t m_xWormHoleOpenTicks = 0;
static const SOUNDFX_SFile* m_sFileWormholeOpen = NULL;

void WORMHOLE_FullStop()
{
    GPIO_ClearAllPixels();
    GPIO_RefreshPixels();
}

void WORMHOLE_Open(const WORMHOLE_SArg* pArg, volatile bool* pIsCancelled)
{
    m_xWormHoleOpenTicks = xTaskGetTickCount();
    SOUNDFX_WormholeOpen();
    m_sFileWormholeOpen = SOUNDFX_GetFile(SOUNDFX_EFILE_5_gateopen_mp3);
    vTaskDelay(pdMS_TO_TICKS(500));

    m_sArg = *pArg;

    DoRing0();
    DoRing1();
    DoRing2();
    DoRing3();
    vTaskDelay(pdMS_TO_TICKS(250));
    DoRing2();
    DoRing1();
    DoRing0();
}

void WORMHOLE_Run(volatile bool* pIsCancelled)
{
    const uint32_t u32MaxBrightness = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WormholeMaxBrightness);

    // Wait until we get stop command or timeout reached.
    TickType_t xStartTicks = xTaskGetTickCount();

    const uint32_t u32OpenTimeS = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_GateOpenedTimeout);

    m_bIsIdlingSoundPlaying = false;
    bool bIsInitalized = false;
    int32_t s32GlitchTempActCount = 0;

    while(!*pIsCancelled && (m_sArg.bNoTimeLimit || (xTaskGetTickCount() - xStartTicks) < pdMS_TO_TICKS(1000*u32OpenTimeS)))
    {
        if ( (xTaskGetTickCount() - m_xWormHoleOpenTicks) > pdMS_TO_TICKS(m_sFileWormholeOpen->u32TimeMS-500) && !m_bIsIdlingSoundPlaying)
        {
            m_bIsIdlingSoundPlaying = true;
            SOUNDFX_WormholeIdling();
        }

        int32_t s32Speed = 20;

        if (m_sArg.eType == WORMHOLE_ETYPE_Strobe)
        {
            static bool bInvert = false;

            for(int i = 0; i < HWCONFIG_WORMHOLELEDS_LEDCOUNT; i++)
            {
                if (bInvert)
                    GPIO_SetPixel(i, 200, 200, 200);
                else
                    GPIO_SetPixel(i, 0, 0, 0);
            }

            bInvert = !bInvert;
        }
        else if (m_sArg.eType == WORMHOLE_ETYPE_Blackhole)
        {
            static int32_t ix = 0;

            const uint32_t u32MaxBrightness2 = 80;

            for(int ppp = 0; ppp < 3; ppp++)
            {
                int32_t ix2 = (ix + ppp*16) % HWCONFIG_WORMHOLELEDS_LEDCOUNT;

                for(int j = 1; j < 5; j++)
                {
                    int32_t ledIndex = (ix2-j);
                    if (ledIndex < 0)
                        ledIndex = HWCONFIG_WORMHOLELEDS_LEDCOUNT + ledIndex;
                    const int32_t z = 5 + (int32_t)(0.2d*(4-j) * u32MaxBrightness2);

                    GPIO_SetPixel(ledIndex, z, 0, z);
                }

                for(int j = 0; j < 5; j++)
                {
                    const int32_t ledIndex = (ix2+j) % HWCONFIG_WORMHOLELEDS_LEDCOUNT;
                    const int32_t z = 5 + (int32_t)(0.25d*j * u32MaxBrightness2);
                    GPIO_SetPixel( ledIndex, z, 0, z);
                }
            }
            ix = (ix + 1) % HWCONFIG_WORMHOLELEDS_LEDCOUNT;
            s32Speed = 40;
        }
        else if (m_sArg.eType == WORMHOLE_ETYPE_NormalSGU ||
                 m_sArg.eType == WORMHOLE_ETYPE_NormalSG1 ||
                 m_sArg.eType == WORMHOLE_ETYPE_Hell ||
                 m_sArg.eType == WORMHOLE_ETYPE_Glitch)
        {
            float minF = 0.10f;
            float maxF = 0.35f;

            if (!bIsInitalized)
            {
                for(int i = 0; i < HWCONFIG_WORMHOLELEDS_LEDCOUNT; i++)
                {
                    m_sLedEffects[i].fOne = (float)(esp_random() % 100) * 0.01f * maxF;
                    m_sLedEffects[i].bUp = false;
                }
                bIsInitalized = true;
            }

            if (m_sArg.eType == WORMHOLE_ETYPE_Glitch)
            {
                if (s32GlitchTempActCount > 0)
                {
                    // Lowest threshold to create a glitched effect
                    minF = 0.0f;
                    maxF = 0.1f;
                    s32Speed = 20;
                    s32GlitchTempActCount--;
                    if (s32GlitchTempActCount == 0)
                    {
                        bIsInitalized = false;
                    }
                }
                else if (esp_random() % 200 == 0)
                {
                    s32GlitchTempActCount = 25 + (esp_random() % 25);
                    bIsInitalized = false;
                }
            }

            for(int i = 0; i < HWCONFIG_WORMHOLELEDS_LEDCOUNT; i++)
            {
                SLedEffect* psLedEffect = &m_sLedEffects[i];

                // Not optimized at all, basically I'm testing many trick until I'm satisfied
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

                if (m_sArg.eType == WORMHOLE_ETYPE_NormalSGU ||
                    m_sArg.eType == WORMHOLE_ETYPE_Glitch)
                    GPIO_SetPixel(i, HELPERMACRO_MIN(psLedEffect->fOne*u32MaxBrightness * fFactor, 255), HELPERMACRO_MIN(psLedEffect->fOne*u32MaxBrightness * fFactor, 255), HELPERMACRO_MIN(psLedEffect->fOne*u32MaxBrightness * fFactor, 255));
                else if (m_sArg.eType == WORMHOLE_ETYPE_NormalSG1)
                    GPIO_SetPixel(i, HELPERMACRO_MAX(psLedEffect->fOne*u32MaxBrightness * fFactor, 16), HELPERMACRO_MAX(psLedEffect->fOne*u32MaxBrightness * fFactor, 16), 16+HELPERMACRO_MIN(psLedEffect->fOne*u32MaxBrightness * fFactor, 255-16));
                else if (m_sArg.eType == WORMHOLE_ETYPE_Hell)
                    GPIO_SetPixel(i, HELPERMACRO_MIN(psLedEffect->fOne*u32MaxBrightness * fFactor, 255), 1, 1);

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
        }

        GPIO_RefreshPixels();
        vTaskDelay(pdMS_TO_TICKS(s32Speed));
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
    SOUNDFX_WormholeClose();
    vTaskDelay(pdMS_TO_TICKS(800));

    DoRing3();
    DoRing2();
    DoRing1();

    // Clear all pixels
    GPIO_ClearAllPixels();
    GPIO_RefreshPixels();
}

bool WORMHOLE_ValidateWormholeType(WORMHOLE_ETYPE eWormholeType)
{
    return eWormholeType >= 0 && (int)eWormholeType < WORMHOLE_ETYPE_Count;
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
