#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdint.h>
#include <stdbool.h>
#include "Servo.h"

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

    SETTINGS_EENTRY_Count
} SETTINGS_EENTRY;

typedef enum
{
    SETTINGS_ESETRET_OK = 0,
    SETTINGS_ESETRET_CannotSet = 1,
    SETTINGS_ESETRET_InvalidRange = 2
} SETTINGS_ESETRET;

void SETTINGS_Init();
void SETTINGS_Load();
void SETTINGS_Save();

int32_t SETTINGS_GetValueInt32(SETTINGS_EENTRY eEntry);
SETTINGS_ESETRET SETTINGS_SetValueInt32(SETTINGS_EENTRY eEntry, bool bIsDryRun, int32_t s32NewValue);

const char* SETTINGS_ExportJSON();
bool SETTINGS_ImportJSON(const char* szJSON);

#endif