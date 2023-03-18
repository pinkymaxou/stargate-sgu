#ifndef _GATESTEPPER_H_
#define _GATESTEPPER_H_

#include "driver/gpio.h"
#include "HWConfig.h"

void GATESTEPPER_Init();

bool GATESTEPPER_MoveTo(int32_t s32Ticks);

#endif