#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void GPIO_Init();

void GPIO_EnableHoldPowerPin(bool bEnabled);

#ifdef __cplusplus
}
#endif

#endif