#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdint.h>
#include <stdbool.h>
#include "Servo.h"
#include <stddef.h>

typedef enum
{
    SETTINGS_EENTRY_ClampLockedPWM = 0,
    SETTINGS_EENTRY_ClampReleasedPWM,
    SETTINGS_EENTRY_RingHomeOffset,
    SETTINGS_EENTRY_RingSlowDelta,
    SETTINGS_EENTRY_RingSymbolBrightness,
    SETTINGS_EENTRY_StepPerRotation,
    SETTINGS_EENTRY_GateOpenedTimeout,
    SETTINGS_EENTRY_HomeMaximumStepTicks,
    SETTINGS_EENTRY_RampOnPercent,
    SETTINGS_EENTRY_RampOffPercent,

    SETTINGS_EENTRY_WSTAIsActive,
    SETTINGS_EENTRY_WSTASSID,
    SETTINGS_EENTRY_WSTAPass,

    SETTINGS_EENTRY_AnimPrelockDelayMS,
    SETTINGS_EENTRY_AnimPostlockDelayMS,
    SETTINGS_EENTRY_AnimPredialDelayMS,

    SETTINGS_EENTRY_Count
} SETTINGS_EENTRY;

typedef enum
{
    SETTINGS_ESETRET_OK = 0,
    SETTINGS_ESETRET_CannotSet = 1,
    SETTINGS_ESETRET_InvalidRange = 2
} SETTINGS_ESETRET;

typedef enum
{
    SETTINGS_EFLAGS_None = 0,
    SETTINGS_EFLAGS_Secret = 1,      // Indicate it cannot be retrieved, only set
    SETTINGS_EFLAGS_NeedsReboot = 2
} SETTINGS_EFLAGS;

#define SETTINGS_GETVALUESTRING_MAXLEN (100)

void SETTINGS_Init();
void SETTINGS_Load();
void SETTINGS_Save();

int32_t SETTINGS_GetValueInt32(SETTINGS_EENTRY eEntry);
SETTINGS_ESETRET SETTINGS_SetValueInt32(SETTINGS_EENTRY eEntry, bool bIsDryRun, int32_t s32NewValue);

void SETTINGS_GetValueString(SETTINGS_EENTRY eEntry, char* out_value, size_t* length);
SETTINGS_ESETRET SETTINGS_SetValueString(SETTINGS_EENTRY eEntry, bool bIsDryRun, const char* szValue);

const char* SETTINGS_ExportJSON();
bool SETTINGS_ImportJSON(const char* szJSON);

#endif