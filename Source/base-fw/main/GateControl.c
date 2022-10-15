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
#include "Wormhole.h"
#include "ClockMode.h"
#include <stdint.h>

#define TAG "GateControl"

#define GATECONTROL_STACKSIZE 2000
#define GATECONTROL_PRIORITY_DEFAULT (tskIDLE_PRIORITY+1)
#define GATECONTROL_COREID 1

typedef struct
{
    TickType_t ttLastTicks;
    bool bLastIsHome;
} SSMHome;

// Private variables
static volatile GATECONTROL_EMODE m_eMode = GATECONTROL_EMODE_Idle;
static volatile GATECONTROL_UModeArg m_uModeArgument;
static volatile bool m_bIsStop = false;

// Control homing and encoder.
static StaticSemaphore_t m_xSemaphoreCreateMutex;
static SemaphoreHandle_t m_xSemaphoreHandle;

static TaskHandle_t m_sGateControlHandle;
static UBaseType_t m_uxPriority;

static int32_t m_s32Count = 0;

static void GateControlTask( void *pvParameters );
static void MoveTo(int32_t* ps32Count, int32_t s32Target);
static void MoveRelative(int32_t s32RelativeTarget);
static int32_t GetAbsoluteSymbolTarget(uint8_t u8SymbolIndex, int32_t s32StepPerRotation);

void GATECONTROL_Init()
{
    m_xSemaphoreHandle = xSemaphoreCreateMutexStatic(&m_xSemaphoreCreateMutex);
    configASSERT( m_xSemaphoreHandle );
}

void GATECONTROL_Start()
{
	// Create task to respond to SPI queries
	if (xTaskCreatePinnedToCore(GateControlTask, "gatecontrol", GATECONTROL_STACKSIZE, (void*)NULL, m_uxPriority, &m_sGateControlHandle, GATECONTROL_COREID) != pdPASS )
	{
		ESP_ERROR_CHECK(ESP_FAIL);
	}
}

static void GateControlTask( void *pvParameters )
{
    ESP_LOGI(TAG, "GateControl task started");

	while(1)
	{
        // Clear the wormhole
        WORMHOLE_FullStop();

        xSemaphoreTake(m_xSemaphoreHandle, portMAX_DELAY);
        const GATECONTROL_EMODE eMode = m_eMode;
        const GATECONTROL_UModeArg uModeArg = m_uModeArgument;
        xSemaphoreGive(m_xSemaphoreHandle);

        m_bIsStop = false;

        if (eMode == GATECONTROL_EMODE_Idle)
        {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        if (eMode == GATECONTROL_EMODE_GoHome)
        {
            GPIO_StartStepper();
            GPIO_ReleaseClamp();
            SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_FadeIn);
            ESP_LOGI(TAG, "Go Home - started");

            SSMHome ssHome = { .ttLastTicks = xTaskGetTickCount(), .bLastIsHome = !gpio_get_level(FWCONFIG_HOMESENSOR_PIN) };

            vTaskDelay(pdMS_TO_TICKS(750));

            // If it cannot after 8000 we consider it failed.
            bool bIsFailed = true;

            m_s32Count = 0;
            const uint32_t u32MaxStep = (uint32_t)NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_HomeMaximumStepTicks);

            for(int i = 0; i < u32MaxStep; i++)
            {
                GPIO_StepMotorCW();
                m_s32Count++;

                const bool bIsHome = !gpio_get_level(FWCONFIG_HOMESENSOR_PIN);
                if (!ssHome.bLastIsHome && bIsHome)
                {
                    ESP_LOGI(TAG, "Go Home - Reached, count: %d", m_s32Count);
                    //ssHome.s32Count = 0;
                    m_s32Count = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_RingHomeOffset);
                    bIsFailed = false;
                    break;
                }
                ssHome.bLastIsHome = bIsHome;
                vTaskDelay(pdMS_TO_TICKS(1));

                if (m_bIsStop)
                {
                    ESP_LOGE(TAG, "Go Home - Cancelled by user");
                    bIsFailed = true;
                    break;
                }
            }

            //
            if (bIsFailed)
            {
                ESP_LOGE(TAG, "Go Home - ERROR! Cannot do homing");
                SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_ErrorToWhite);
            }
            else
            {
                ESP_LOGI(TAG, "Go Home - Readjustment in progress");
                MoveTo(&m_s32Count, 0);
            }

            GPIO_LockClamp();
            vTaskDelay(pdMS_TO_TICKS(250));
            GPIO_StopClamp();
            GPIO_StopStepper();

            ESP_LOGI(TAG, "Go Home - ended");
        }
        else if (eMode == GATECONTROL_EMODE_Dial)
        {
            bool bIsSuccess = true;

            GPIO_SetRampLightOnOff(true);
            WORMHOLE_FullStop();

            GPIO_StartStepper();
            GPIO_ReleaseClamp();
            SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_FadeIn);
            vTaskDelay(pdMS_TO_TICKS(NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_AnimPredialDelayMS)));

            const int32_t s32StepPerRotation = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_StepPerRotation);
            const uint32_t u32SymbBright = (uint32_t)NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_RingSymbolBrightness);

            for(int i = 0; i < uModeArg.sDialArg.u8SymbolCount; i++)
            {
                if (m_bIsStop)
                {
                    bIsSuccess = false;
                    break;
                }

                const uint8_t u8SymbolIndex = uModeArg.sDialArg.u8Symbols[i];
                int32_t s32TicksTarget = GetAbsoluteSymbolTarget(u8SymbolIndex, s32StepPerRotation);

                ESP_LOGI(TAG, "Go Home - Symbol: %d, previous target: %d, new target: %d, current: %d", /*0*/u8SymbolIndex, /*1*/m_s32Count, /*2*/s32TicksTarget, /*3*/m_s32Count);

                int32_t s32Move = (s32TicksTarget - m_s32Count);

                int32_t s32Move2 = -1 * ((s32StepPerRotation - s32TicksTarget) + m_s32Count);
                if (abs(s32Move2) < abs(s32Move))
                    s32Move = s32Move2;
                int32_t s32Move3 = ((s32StepPerRotation - m_s32Count) + s32TicksTarget);
                if (abs(s32Move3) < abs(s32Move))
                    s32Move = s32Move3;

                MoveRelative(s32Move);
                m_s32Count = s32TicksTarget;

                vTaskDelay(pdMS_TO_TICKS(NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_AnimPrelockDelayMS)));
                SGUBRCOMM_LightUpLED(&g_sSGUBRCOMMHandle, u32SymbBright, u32SymbBright, u32SymbBright, SGUHELPER_SymbolIndexToLedIndex(u8SymbolIndex));
                vTaskDelay(pdMS_TO_TICKS(NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_AnimPostlockDelayMS)));
            }

            // Stop stepper and servos ...
            GPIO_StopStepper();
            GPIO_StopClamp();

            if (bIsSuccess)
            {
                ESP_LOGI(TAG, "Dial done!");

                WORMHOLE_Open(&m_bIsStop);
                WORMHOLE_Run(&m_bIsStop, false);
                WORMHOLE_Close(&m_bIsStop);
            }
            else
            {
                ESP_LOGE(TAG, "Unable to complete dial process");
            }

            SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_FadeOut);
            GPIO_SetRampLightOnOff(false);
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
            WORMHOLE_Open(&m_bIsStop);
            WORMHOLE_Run(&m_bIsStop, true);
            WORMHOLE_Close(&m_bIsStop);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
        m_eMode = GATECONTROL_EMODE_Idle;
	}
	vTaskDelete( NULL );
}

static int32_t GetAbsoluteSymbolTarget(uint8_t u8SymbolIndex, int32_t s32StepPerRotation)
{
    const uint32_t u32SymbolLedIndex = SGUHELPER_SymbolIndexToLedIndex(u8SymbolIndex);
    const double dblPercent = (SGUHELPER_LEDIndexToDeg(u32SymbolLedIndex) / 360.0d);
    return dblPercent * (double)s32StepPerRotation;
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

        if (puModeArg->sDialArg.u8SymbolCount < 1 || puModeArg->sDialArg.u8SymbolCount > 9)
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
    else if (eMode == GATECONTROL_EMODE_Stop)
    {
        // Request current process to stop
        m_bIsStop = true;
        goto END;
    }

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
    const uint32_t u32SlowDelta = (uint32_t)NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_RingSlowDelta);

    int32_t s32Target = abs(s32RelativeTarget);

    if (s32RelativeTarget > 0)
    {
        ESP_LOGI(TAG, "Go Home - Ajustment clockwise");

        int accel = u32SlowDelta;
        while(s32Target > 0)
        {
            GPIO_StepMotorCW();
            s32Target--;

            if (accel > 0)
                accel--;
            vTaskDelay(pdMS_TO_TICKS((accel > 0 || s32Target < u32SlowDelta) ? 3 : 1));
        }
    }
    else if (s32RelativeTarget < 0)
    {
        ESP_LOGI(TAG, "Go Home - Ajustment counter clockwise");

        int accel = u32SlowDelta;
        while(s32Target > 0)
        {
            GPIO_StepMotorCCW();
            s32Target--;

            if (accel > 0)
                accel--;
            vTaskDelay(pdMS_TO_TICKS((accel > 0 || s32Target < u32SlowDelta) ? 3 : 1));
        }
    }
}
