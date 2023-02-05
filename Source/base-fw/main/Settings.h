#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdint.h>
#include <stdbool.h>
#include "Servo.h"
#include <stddef.h>
#include "nvsjson.h"

typedef enum
{
    SETTINGS_EENTRY_ClampLockedPWM = 0,
    SETTINGS_EENTRY_ClampReleasedPWM,
    SETTINGS_EENTRY_RingHomeOffset,
    SETTINGS_EENTRY_RingSymbolBrightness,

    SETTINGS_EENTRY_StepPerRotation,
    SETTINGS_EENTRY_TimePerRotationMS,
    SETTINGS_EENTRY_GateOpenedTimeout,

    SETTINGS_EENTRY_RampOnPercent,

    SETTINGS_EENTRY_WormholeMaxBrightness,
    SETTINGS_EENTRY_WormholeType,

    SETTINGS_EENTRY_WSTAIsActive,
    SETTINGS_EENTRY_WSTASSID,
    SETTINGS_EENTRY_WSTAPass,

    SETTINGS_EENTRY_AnimPrelockDelayMS,
    SETTINGS_EENTRY_AnimPostlockDelayMS,
    SETTINGS_EENTRY_AnimPredialDelayMS,

    SETTINGS_EENTRY_Count
} SETTINGS_EENTRY;

extern NVSJSON_SHandle g_sSettingHandle;

void SETTINGS_Init();

#endif