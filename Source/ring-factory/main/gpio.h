#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdbool.h>
#include <stdint.h>

void GPIO_Init();

void GPIO_SetPixel(uint32_t u32Index, uint8_t u8Red, uint8_t u8Green, uint8_t u8Blue);
void GPIO_ClearAllPixels();
void GPIO_RefreshPixels();

#endif