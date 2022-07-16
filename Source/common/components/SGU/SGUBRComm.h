#ifndef _SGUBRCOMM_H_
#define _SGUBRCOMM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "SGUBRProtocol.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/Task.h"
#include "freertos/semphr.h"

typedef enum
{
    SGUBRCOMM_EMODE_Ring,
    SGUBRCOMM_EMODE_Base,
} SGUBRCOMM_EMODE;

typedef struct
{
    SGUBRCOMM_EMODE eMode;
    const SGUBRPROTOCOL_SConfig* psSGUBRProtocolConfig;

    char dstRingAddress[32]; 
} SGUBRCOMM_SSetting;

typedef struct
{
    const SGUBRCOMM_SSetting* psSetting;

    // Receive data
    uint8_t u8Buffers[64];
    int sock;

    long lAutoOffTicks;

    StaticSemaphore_t sSemaphoreCreateMutex;
    SemaphoreHandle_t sSemaphoreHandle;

    SGUBRPROTOCOL_SHandle sSGUBRProtHandle;
} SGUBRCOMM_SHandle;

void SGUBRCOMM_Init(SGUBRCOMM_SHandle* pHandle, const SGUBRCOMM_SSetting* psSetting);

void SGUBRCOMM_Start(SGUBRCOMM_SHandle* pHandle);

void SGUBRCOMM_Process(SGUBRCOMM_SHandle* pHandle);

void SGUBRCOMM_ChevronLightning(SGUBRCOMM_SHandle* pHandle, SGUBRPROTOCOL_ECHEVRONANIM eChevronAnim);

void SGUBRCOMM_TurnOff(SGUBRCOMM_SHandle* pHandle);

void SGUBRCOMM_GotoFactory(SGUBRCOMM_SHandle* pHandle);

void SGUBRCOMM_LightUpLED(SGUBRCOMM_SHandle* pHandle, uint8_t u8Red, uint8_t u8Green, uint8_t u8Blue, uint8_t u8LedIndex);

#ifdef __cplusplus
}
#endif

#endif