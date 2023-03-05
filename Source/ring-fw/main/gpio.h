#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdint.h>
#include <stdbool.h>

void GPIO_Init();

void GPIO_EnableHoldPowerPin(bool bEnabled);

void GPIO_SetPixel(uint32_t u32Index, uint8_t u8Red, uint8_t u8Green, uint8_t u8Blue);
void GPIO_ClearAllPixels();
void GPIO_RefreshPixels();

#endif