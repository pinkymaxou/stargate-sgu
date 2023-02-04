#include "Settings.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <assert.h>
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>
#include "Wormhole.h"

#define TAG "SETTINGS"

static bool ValidateWifiPassword(const NVSJSON_SSettingEntry* pSettingEntry, const char* szValue);

static NVSJSON_SSettingEntry m_sConfigEntries[SETTINGS_EENTRY_Count] =
{
    [SETTINGS_EENTRY_ClampLockedPWM] =          NVSJSON_INITINT32_RNG("Clamp.LockedPWM", "Servo motor locked PWM",             1250, 1000,   2000, NVSJSON_EFLAGS_None),
    [SETTINGS_EENTRY_ClampReleasedPWM] =        NVSJSON_INITINT32_RNG("Clamp.ReleasPWM", "Servo motor released PWM",           1000, 1000,   2000, NVSJSON_EFLAGS_None),
    [SETTINGS_EENTRY_RingHomeOffset] =          NVSJSON_INITINT32_RNG("Ring.HomeOffset", "Offset relative to home sensor",      -55, -500,    500, NVSJSON_EFLAGS_None),
    [SETTINGS_EENTRY_RingSlowDelta] =           NVSJSON_INITINT32_RNG("Ring.SlowDelta",  "Encoder delta in slow move mode",     300,    0,    800, NVSJSON_EFLAGS_None),
    [SETTINGS_EENTRY_RingSymbolBrightness] =    NVSJSON_INITINT32_RNG("Ring.SymBright",  "Symbol brightness",                    15,    3,     50, NVSJSON_EFLAGS_None),

    [SETTINGS_EENTRY_StepPerRotation] =         NVSJSON_INITINT32_RNG("StepPerRot",      "How many step per rotation",         7334,    0,  64000, NVSJSON_EFLAGS_None),
    [SETTINGS_EENTRY_TimePerRotationMS] =       NVSJSON_INITINT32_RNG("TimePerRot",      "Time to do a rotation",                 0,    0, 120000, NVSJSON_EFLAGS_None),
    [SETTINGS_EENTRY_GateOpenedTimeout] =       NVSJSON_INITINT32_RNG("GateTimeoutS",    "Timeout (s) before the gate close",   300,   10,  42*60, NVSJSON_EFLAGS_None),

    [SETTINGS_EENTRY_RampOnPercent] =           NVSJSON_INITINT32_RNG("Ramp.LightOn",    "Ramp illumination ON (percent)",       30,    0,    100, NVSJSON_EFLAGS_None),

    [SETTINGS_EENTRY_WormholeMaxBrightness] =   NVSJSON_INITINT32_RNG("WH.MaxBright",    "Maximum brightness for wormhole leds. (Warning: can cause voltage drop)", 180, 0, 255, NVSJSON_EFLAGS_None),
    [SETTINGS_EENTRY_WormholeType] =            NVSJSON_INITINT32_RNG("WH.Type",         "0: SGU, 1: SG1, 2: Hell", (int)WORMHOLE_ETYPE_NormalSGU, 0, (int)WORMHOLE_ETYPE_Count-1, NVSJSON_EFLAGS_None),

    // WiFi Station related
    [SETTINGS_EENTRY_WSTAIsActive] =            NVSJSON_INITINT32_RNG("WSTA.IsActive",   "Wi-Fi is active",                        0,    0, 1, NVSJSON_EFLAGS_NeedsReboot),
    [SETTINGS_EENTRY_WSTASSID] =                NVSJSON_INITSTRING_RNG("WSTA.SSID",      "Wi-Fi (SSID)",                           "", NVSJSON_EFLAGS_NeedsReboot),
    [SETTINGS_EENTRY_WSTAPass] =                NVSJSON_INITSTRING_VALIDATOR("WSTA.Pass","Wi-Fi password",                         "", ValidateWifiPassword, NVSJSON_EFLAGS_Secret | NVSJSON_EFLAGS_NeedsReboot),

    // Animation delay
    [SETTINGS_EENTRY_AnimPrelockDelayMS] =      NVSJSON_INITINT32_RNG("dial.anim1",      "Delay before locking the chevron (ms)",  750, 0, 6000, NVSJSON_EFLAGS_None),
    [SETTINGS_EENTRY_AnimPostlockDelayMS] =     NVSJSON_INITINT32_RNG("dial.anim2",      "Delay after locking the chevron (ms)",  1250, 0, 6000, NVSJSON_EFLAGS_None),
    [SETTINGS_EENTRY_AnimPredialDelayMS] =      NVSJSON_INITINT32_RNG("dial.anim3",      "Delay before starting to dial (ms)",    2500, 0, 10000, NVSJSON_EFLAGS_None),
};

NVSJSON_SHandle g_sSettingHandle;

static nvs_handle_t m_sNVS;

void SETTINGS_Init()
{
    NVSJSON_Init(&g_sSettingHandle, m_sConfigEntries, SETTINGS_EENTRY_Count);
}

static bool ValidateWifiPassword(const NVSJSON_SSettingEntry* pSettingEntry, const char* szValue)
{
    const size_t n = strlen(szValue);
    return n > 8 || n == 0;
}
