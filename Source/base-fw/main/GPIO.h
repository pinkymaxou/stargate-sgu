#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdbool.h>
#include <stdint.h>

void GPIO_Init();

// Private functions
void GPIO_StepMotorCW();
void GPIO_StepMotorCCW();
void GPIO_StartStepper();
void GPIO_StopStepper();

void GPIO_ReleaseClamp();
void GPIO_LockClamp();
void GPIO_StopClamp();

bool GPIO_IsHomeActive();

void GPIO_SetRampLightPerc(float fltPercent);
void GPIO_SetSanityLEDStatus(bool bIsLightUp);

void GPIO_SetPixel(uint32_t u32Index, uint8_t u8Red, uint8_t u8Green, uint8_t u8Blue);
void GPIO_ClearAllPixels();
void GPIO_RefreshPixels();

void GPIO_SendMp3PlayerCMD(const char* szCmd);

#endif