#include "GateControl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "FWConfig.h"
#include "GPIO.h"
#include "Settings.h"
#include "driver/gpio.h"
#include "SGUBRComm.h"
#include "SGUBRProtocol.h"
#include "SGUHelper.h"
#include "base-fw.h"
#include "ClockMode.h"
#include "GateControl.h"
#include "GateStepper.h"
#include "SoundFX.h"
#include <stdint.h>
#include <string.h>

#define TAG "GateControl"

// Private variables
static volatile GATECONTROL_EMODE m_eMode = GATECONTROL_EMODE_Idle;
static volatile GATECONTROL_UModeArg m_uModeArgument;
static volatile bool m_bIsStop = false;

// Control homing and encoder.
static StaticSemaphore_t m_xSemaphoreCreateMutex;
static SemaphoreHandle_t m_xSemaphoreHandle;

static TaskHandle_t m_sGateControlHandle;

static int32_t m_s32Count = 0;

static bool m_bIsHomingCompleted = false;

static void GateControlTask( void *pvParameters );

static void MoveTo(int32_t* ps32Count, int32_t s32Target);
static void MoveRelative(int32_t s32RelativeTarget);
static bool MoveUntilHomeSwitch(bool bHomeSwitchState, uint32_t u32TimeOutMS, int32_t* ps32Count);

static int32_t GetAbsoluteSymbolTarget(uint8_t u8SymbolIndex, int32_t s32StepPerRotation);

static bool AutoCalibrate();
static bool DoHoming();
static bool DoDialSequence(const GATECONTROL_SDialArg* psDialArg);

void GATECONTROL_Init()
{
    m_xSemaphoreHandle = xSemaphoreCreateMutexStatic(&m_xSemaphoreCreateMutex);
    configASSERT( m_xSemaphoreHandle );
}

void GATECONTROL_Start()
{
	// Create task to respond to SPI queries
	if (xTaskCreatePinnedToCore(GateControlTask, "gatecontrol", FWCONFIG_GATECONTROL_STACKSIZE, (void*)NULL, FWCONFIG_GATECONTROL_PRIORITY_DEFAULT, &m_sGateControlHandle, FWCONFIG_GATECONTROL_COREID) != pdPASS )
	{
		ESP_ERROR_CHECK(ESP_FAIL);
	}
}

static void GateControlTask( void *pvParameters )
{
    ESP_LOGI(TAG, "GateControl task started");

    GATECONTROL_DoAction(GATECONTROL_EMODE_GoHome, NULL);
    // GATECONTROL_AnimRampLight(true);

	while(1)
	{
        // Clear the wormhole
        WORMHOLE_FullStop();

        xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
        const GATECONTROL_EMODE eMode = m_eMode;
        const GATECONTROL_UModeArg uModeArg = m_uModeArgument;

        // Reset temporary vars ...
        m_eMode = GATECONTROL_EMODE_Idle;
        memset((void*)&m_uModeArgument, 0, sizeof(m_uModeArgument));
        m_bIsStop = false;

        xSemaphoreGive(m_xSemaphoreHandle);

        if (eMode == GATECONTROL_EMODE_Idle)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (eMode == GATECONTROL_EMODE_GoHome)
        {
            SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_FadeIn);
            vTaskDelay(pdMS_TO_TICKS(750));
            if (DoHoming())
                SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_FadeOut);
            else
                SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_ErrorToOff);
        }
        else if (eMode == GATECONTROL_EMODE_AutoCalibrate)
        {
            if (AutoCalibrate())
            {
                DoHoming();
            }
        }
        else if (eMode == GATECONTROL_EMODE_Dial)
        {
            if (DoDialSequence(&uModeArg.sDialArg))
            {
                // Go back to home ...
                vTaskDelay(pdMS_TO_TICKS(5000));

                // We give a chance to home ...
                m_bIsStop = false;
                DoHoming();
            }

        }
        else if (eMode == GATECONTROL_EMODE_ActiveClock)
        {
            ESP_LOGI(TAG, "GateControl activate clock mode");
            CLOCKMODE_Run(&m_bIsStop);
        }
        else if (eMode == GATECONTROL_EMODE_ManualReleaseClamp)
        {
            ESP_LOGI(TAG, "GateControl release clamp");
            GPIO_ReleaseClamp();
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        else if (eMode == GATECONTROL_EMODE_ManualLockClamp)
        {
            ESP_LOGI(TAG, "GateControl lock clamp");
            GPIO_LockClamp();
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        else if (eMode == GATECONTROL_EMODE_ManualWormhole)
        {
            ESP_LOGI(TAG, "GateControl manual wormhole");
            const WORMHOLE_SArg sArg = { .eType = uModeArg.sManualWormhole.eWormholeType, .bNoTimeLimit = true };
            WORMHOLE_Open(&sArg, &m_bIsStop);
            WORMHOLE_Run(&m_bIsStop);
            WORMHOLE_Close(&m_bIsStop);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
	}
	vTaskDelete( NULL );
}

static int32_t GetAbsoluteSymbolTarget(uint8_t u8SymbolIndex, int32_t s32StepPerRotation)
{
    const uint32_t u32SymbolLedIndex = SGUHELPER_SymbolIndexToLedIndex(u8SymbolIndex);
    const double dblPercent = (SGUHELPER_LEDIndexToDeg(u32SymbolLedIndex) / 360.0d);
    return dblPercent * (double)s32StepPerRotation * -1;
}

bool GATECONTROL_DoAction(GATECONTROL_EMODE eMode, const GATECONTROL_UModeArg* puModeArg)
{
    xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);

    if (eMode == GATECONTROL_EMODE_Dial)
    {
        if (puModeArg == NULL)
        {
            ESP_LOGE(TAG, "No argument provided");
            goto ERROR;
        }

        if (!WORMHOLE_ValidateWormholeType(puModeArg->sDialArg.eWormholeType))
        {
            ESP_LOGE(TAG, "Invalid wormhole type");
            goto ERROR;
        }

        if (puModeArg->sDialArg.u8SymbolCount < 1 || puModeArg->sDialArg.u8SymbolCount > (sizeof(puModeArg->sDialArg.u8Symbols)/sizeof(puModeArg->sDialArg.u8Symbols[0])))
        {
            ESP_LOGE(TAG, "Invalid symbol count");
            goto ERROR;
        }

        for(int i = 0; i < puModeArg->sDialArg.u8SymbolCount; i++)
        {
            if (puModeArg->sDialArg.u8Symbols[i] < 1 || puModeArg->sDialArg.u8Symbols[i] > 36)
            {
                ESP_LOGE(TAG, "Invalid symbol");
                goto ERROR;
            }
        }
    }
    else if (eMode == GATECONTROL_EMODE_ManualWormhole)
    {
        if (puModeArg == NULL)
        {
            ESP_LOGE(TAG, "No argument provided");
            goto ERROR;
        }

        if (!WORMHOLE_ValidateWormholeType(puModeArg->sManualWormhole.eWormholeType))
        {
            ESP_LOGE(TAG, "Invalid wormhole type");
            goto ERROR;
        }
    }
    else if (eMode == GATECONTROL_EMODE_Stop)
    {
        // Request current process to stop
        m_bIsStop = true;
        goto END;
    }

    // Stop what it was doing before
    m_bIsStop = true;
    m_eMode = eMode;
    if (puModeArg != NULL)
        m_uModeArgument = *puModeArg;
    END:
    xSemaphoreGive(m_xSemaphoreHandle);
    return true;
    ERROR:
    xSemaphoreGive(m_xSemaphoreHandle);
    return false;
}

static void MoveTo(int32_t* ps32Count, int32_t s32Target)
{
    int32_t s32Rel = s32Target - *ps32Count;
    MoveRelative(s32Rel);
    *ps32Count = *ps32Count + s32Rel;
}

static void MoveRelative(int32_t s32RelativeTarget)
{
    GATESTEPPER_MoveTo(s32RelativeTarget);
}

static bool AutoCalibrate()
{
    const char* szError = "Unknown";

    ESP_LOGI(TAG, "[Autocalibration] started");

    // Move until it's outside
    GPIO_StartStepper();
    GPIO_ReleaseClamp();

    const int32_t s32AttemptCount = 10;
    int32_t s32StepCount = 0;

    for(int i = 0; i < s32AttemptCount; i++)
    {
        if (m_bIsStop)
        {
            szError = "Cancelled by user";
            goto ERROR;
        }

        // Move until the home detection is active ...
        MoveUntilHomeSwitch(true, 30000, NULL);
        // Move until the home detection is off ...
        MoveUntilHomeSwitch(false, 30000, NULL);

        if (m_bIsStop)
        {
            szError = "Cancelled by user";
            goto ERROR;
        }

        // Move until the home detection is off ...
        int32_t s32Count = 0;
        MoveUntilHomeSwitch(true, 30000, &s32Count);
        int32_t s32CountGap = 0;
        MoveUntilHomeSwitch(false, 30000, &s32CountGap);

        s32StepCount += s32Count + s32CountGap;
        ESP_LOGI(TAG, "[Autocalibration] #%d, count: %d, gap: %d, tick count: %d", i+1, s32Count, s32CountGap, s32Count + s32CountGap);
    }

    const int32_t s32OldStepPerRotation = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_StepPerRotation);
    const int32_t s32NewStepPerRotation = s32StepCount / s32AttemptCount;

    // Count how long it takes to do one rotation
    const TickType_t ttStartAll = xTaskGetTickCount();
    MoveRelative(s32NewStepPerRotation);
    const int32_t s32NewTimePerRotation = pdTICKS_TO_MS(xTaskGetTickCount() - ttStartAll);

    ESP_LOGI(TAG, "[Autocalibration] After %d turn, it got: %d, avg: %.2f, ticks per rotation: %d => %d, rotation time: %d ms",
        s32AttemptCount, s32StepCount, (float)s32StepCount / (float)s32AttemptCount,
        s32OldStepPerRotation, s32NewStepPerRotation,
        s32NewTimePerRotation );

    // Record values
    NVSJSON_SetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_StepPerRotation, false, s32NewStepPerRotation);
    NVSJSON_SetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_TimePerRotationMS, false, s32NewTimePerRotation);

    NVSJSON_Save(&g_sSettingHandle);
    return true;
    ERROR:
    ESP_LOGE(TAG, "[Autocalibration] Error: %s", szError);
    return false;
}

static bool MoveUntilHomeSwitch(bool bHomeSwitchState, uint32_t u32TimeOutMS, int32_t* ps32Count)
{
    const char* szError = "Unknown";

    TickType_t ttStart = xTaskGetTickCount();
    while(1)
    {
        if ((xTaskGetTickCount() - ttStart) > pdMS_TO_TICKS(u32TimeOutMS))
        {
            szError = "Timeout";
            goto ERROR;
        }

        if (GPIO_IsHomeActive() == bHomeSwitchState)
        {
            ESP_LOGI(TAG, "[Autocalibration] state reached %d", (int)bHomeSwitchState);
            break;
        }
        GPIO_StepMotorCW();
        if (ps32Count != NULL)
            *ps32Count = *ps32Count + 1;
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    return true;
    ERROR:
    ESP_LOGE(TAG, "MoveUntilHomeSwitch error: %s", szError);
    return false;
}

static bool DoHoming()
{
    const char* szErrorString = "unknown";

    m_bIsHomingCompleted = false;

    GPIO_StartStepper();
    GPIO_ReleaseClamp();
    ESP_LOGI(TAG, "[DoHoming] Started");
    const uint32_t u32StepPerRotation = (uint32_t)NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_StepPerRotation);

    if (u32StepPerRotation == 0)
    {
        szErrorString = "HomeMaximumStepTicks not set";
        goto ERROR;
    }

    vTaskDelay(pdMS_TO_TICKS(750));

    m_s32Count = 0;

    // Fast mode, if
    const bool bIsFastMode = GPIO_IsHomeActive();

    if (bIsFastMode)
    {
        ESP_LOGI(TAG, "Homing using fast mode algorithm");

        /* If the Gate is already near the homing sensor we can just move out of the sensor then move near the sensor again
           as homing. */
        while(1)
        {
            if (m_bIsStop)
            {
                szErrorString = "Cancelled by user";
                goto ERROR;
            }

            if (abs(m_s32Count) >= (uint32_t)(u32StepPerRotation * 1.1))
            {
                szErrorString = "homing timeout";
                goto ERROR;
            }

            GPIO_StepMotorCCW();
            m_s32Count--;

            if (!GPIO_IsHomeActive())
            {
                ESP_LOGI(TAG, "Homing first step, count: %d", m_s32Count);
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        while(1)
        {
            if (m_bIsStop)
            {
                szErrorString = "Cancelled by user";
                goto ERROR;
            }

            if (abs(m_s32Count) >= (uint32_t)(u32StepPerRotation * 1.1))
            {
                szErrorString = "homing timeout";
                goto ERROR;
            }

            GPIO_StepMotorCW();
            m_s32Count++;

            if (GPIO_IsHomeActive())
            {
                ESP_LOGI(TAG, "[DoHoming] Reached, count: %d", m_s32Count);
                m_s32Count = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_RingHomeOffset);
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    else
    {
        ESP_LOGI(TAG, "Homing using slow mode algorithm");

        // We allow 10% error maximum
        bool bLastIsHome = GPIO_IsHomeActive();

        while(1)
        {
            if (m_s32Count >= (uint32_t)(u32StepPerRotation * 1.1))
            {
                szErrorString = "homing timeout";
                goto ERROR;
            }

            if (m_bIsStop)
            {
                szErrorString = "Cancelled by user";
                goto ERROR;
            }

            GPIO_StepMotorCW();
            m_s32Count++;

            const bool bIsHome = GPIO_IsHomeActive();
            if (!bLastIsHome && bIsHome)
            {
                ESP_LOGI(TAG, "[DoHoming] Reached, count: %d", m_s32Count);
                m_s32Count = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_RingHomeOffset);
                break;
            }
            bLastIsHome = bIsHome;
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

    // Move back to reajusted home
    ESP_LOGI(TAG, "[DoHoming] - Readjustment in progress");
    MoveTo(&m_s32Count, 0);

    GPIO_LockClamp();
    vTaskDelay(pdMS_TO_TICKS(250));
    GPIO_StopClamp();
    GPIO_StopStepper();
    ESP_LOGI(TAG, "[DoHoming] - ended");

    m_bIsHomingCompleted = true;
    return true;
    ERROR:
    ESP_LOGE(TAG, "[DoHoming] error: %s", szErrorString);
    return false;
}

void GATECONTROL_AnimRampLight(bool bIsActive)
{
    const float fltPWMOn = (float)NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_RampOnPercent) / 100.0f;
    const float fltInc = (fltPWMOn / 50.0f);

    if (bIsActive)
        for(float flt = 0.0f; flt < fltPWMOn; flt += fltInc)
        {
            GPIO_SetRampLightPerc(flt);
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    else
        for(float flt = fltPWMOn; flt >= 0.0f; flt -= fltInc)
        {
            GPIO_SetRampLightPerc(flt);
            vTaskDelay(pdMS_TO_TICKS(5));
        }
}

static bool DoDialSequence(const GATECONTROL_SDialArg* psDialArg)
{
    const char* szErrorString = "unknown";

    if (!m_bIsHomingCompleted)
    {
        szErrorString = "homing is not completed";
        goto ERROR;
    }

    // Animation off ...
    GATECONTROL_AnimRampLight(true);
    WORMHOLE_FullStop();
    SOUNDFX_Stop();

    GPIO_StartStepper();
    GPIO_ReleaseClamp();

    SOUNDFX_ActivateGate();

    SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_FadeIn);
    vTaskDelay(pdMS_TO_TICKS(NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_AnimPredialDelayMS)));

    const int32_t s32StepPerRotation = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_StepPerRotation);
    const uint32_t u32SymbBright = (uint32_t)NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_RingSymbolBrightness);

    for(int i = 0; i < psDialArg->u8SymbolCount; i++)
    {
        if (m_bIsStop)
        {
            szErrorString = "cancelled by user";
            goto ERROR;
        }

        const uint8_t u8SymbolIndex = psDialArg->u8Symbols[i];
        int32_t s32TicksTarget = GetAbsoluteSymbolTarget(u8SymbolIndex, s32StepPerRotation);

        ESP_LOGI(TAG, "Go Home - Symbol: %d, previous target: %d, new target: %d, current: %d", /*0*/u8SymbolIndex, /*1*/m_s32Count, /*2*/s32TicksTarget, /*3*/m_s32Count);

        int32_t s32Move = (s32TicksTarget - m_s32Count);

        const int32_t s32Move2 = -1 * ((s32StepPerRotation - s32TicksTarget) + m_s32Count);
        if (abs(s32Move2) < abs(s32Move))
            s32Move = s32Move2;
        const int32_t s32Move3 = ((s32StepPerRotation - m_s32Count) + s32TicksTarget);
        if (abs(s32Move3) < abs(s32Move))
            s32Move = s32Move3;

        SOUNDFX_StartRollingSound();
        vTaskDelay(pdMS_TO_TICKS(250));
        MoveRelative(s32Move);
        SOUNDFX_Stop();
        vTaskDelay(pdMS_TO_TICKS(150));
        SOUNDFX_EngageChevron();

        m_s32Count = s32TicksTarget;

        vTaskDelay(pdMS_TO_TICKS(NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_AnimPrelockDelayMS)));
        SGUBRCOMM_LightUpLED(&g_sSGUBRCOMMHandle, u32SymbBright, u32SymbBright, u32SymbBright, SGUHELPER_SymbolIndexToLedIndex(u8SymbolIndex));
        SOUNDFX_ChevronLock();
        vTaskDelay(pdMS_TO_TICKS(NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_AnimPostlockDelayMS)));
    }

    // Stop stepper and servos ...
    GPIO_StopStepper();
    GPIO_StopClamp();

    ESP_LOGI(TAG, "Dial done!");

    const WORMHOLE_SArg sArg = { .eType = psDialArg->eWormholeType, .bNoTimeLimit = false };
    WORMHOLE_Open(&sArg, &m_bIsStop);
    WORMHOLE_Run(&m_bIsStop);
    WORMHOLE_Close(&m_bIsStop);

    SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_FadeOut);
    GATECONTROL_AnimRampLight(false);
    return true;
    ERROR:
    ESP_LOGE(TAG, "unable to dial: %s", szErrorString);
    GPIO_StopClamp();
    GPIO_StopStepper();
    SOUNDFX_Fail();
    GATECONTROL_AnimRampLight(false);
    SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_ErrorToOff);
    return false;
}