#ifndef _GATECONTROL_H
#define _GATECONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include "Wormhole.h"

typedef enum
{
    GATECONTROL_EMODE_Idle,
    GATECONTROL_EMODE_GoHome,
    GATECONTROL_EMODE_ManualReleaseClamp,
    GATECONTROL_EMODE_ManualLockClamp,
    GATECONTROL_EMODE_Stop,
    GATECONTROL_EMODE_Dial,
    GATECONTROL_EMODE_AutoCalibrate,

    GATECONTROL_EMODE_ManualWormhole,
    GATECONTROL_EMODE_ActiveClock,
} GATECONTROL_EMODE;

typedef struct
{
    uint8_t u8Symbols[36];
    uint8_t u8SymbolCount;
    WORMHOLE_ETYPE eWormholeType;
} GATECONTROL_SDialArg;

typedef union
{
    GATECONTROL_SDialArg sDialArg;
    struct
    {
        WORMHOLE_ETYPE eWormholeType;
    } sManualWormhole;
} GATECONTROL_UModeArg;


typedef struct
{
    GATECONTROL_EMODE eMode;

    char szStatusText[150+1];

    bool bHasLastError;
    char szLastError[150+1];

    bool bIsCancelRequested;
} GATECONTROL_SState;

void GATECONTROL_Init();

void GATECONTROL_Process();

void GATECONTROL_Start();

bool GATECONTROL_DoAction(GATECONTROL_EMODE eMode, const GATECONTROL_UModeArg* puModeArg);

void GATECONTROL_AnimRampLight(bool bIsActive);

void GATECONTROL_GetState(GATECONTROL_SState* pState);

#endif