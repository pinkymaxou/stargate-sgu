#ifndef _GATECONTROL_H
#define _GATECONTROL_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    GATECONTROL_EMODE_Idle,
    GATECONTROL_EMODE_GoHome,
    GATECONTROL_EMODE_ManualReleaseClamp,
    GATECONTROL_EMODE_ManualLockClamp,
    GATECONTROL_EMODE_Stop,
    GATECONTROL_EMODE_Dial,

    GATECONTROL_EMODE_ManualWormhole,
} GATECONTROL_EMODE;

typedef union
{
    struct
    {
        uint8_t u8Symbols[9];
        uint8_t u8SymbolCount;
    } sDialArg;
} GATECONTROL_UModeArg;


void GATECONTROL_Init();

void GATECONTROL_Process();

void GATECONTROL_Start();

bool GATECONTROL_DoAction(GATECONTROL_EMODE eMode, const GATECONTROL_UModeArg* puModeArg);

#endif