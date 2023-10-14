#ifndef _SGUBRPROTOCOL_H_
#define _SGUBRPROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    SGUBRPROTOCOL_ECMD_KeepAlive = 0,
    SGUBRPROTOCOL_ECMD_TurnOff = 1,
    SGUBRPROTOCOL_ECMD_UpdateLight = 2,
    SGUBRPROTOCOL_ECMD_ChevronsLightning = 3,
    SGUBRPROTOCOL_ECMD_GotoFactory = 4,
    SGUBRPROTOCOL_ECMD_GotoOTAMode = 5,
} SGUBRPROTOCOL_ECMD;

typedef struct
{
    uint8_t u8Red;
    uint8_t u8Green;
    uint8_t u8Blue;
} SGUBRPROTOCOL_SRGB;

typedef enum
{
    SGUBRPROTOCOL_ECHEVRONANIM_FadeIn = 0,
    SGUBRPROTOCOL_ECHEVRONANIM_FadeOut = 1,
    SGUBRPROTOCOL_ECHEVRONANIM_ErrorToWhite = 2,
    SGUBRPROTOCOL_ECHEVRONANIM_ErrorToOff = 3,
    SGUBRPROTOCOL_ECHEVRONANIM_AllSymbolsOn = 4,
    SGUBRPROTOCOL_ECHEVRONANIM_Suicide = 5,

    SGUBRPROTOCOL_ECHEVRONANIM_Count
} SGUBRPROTOCOL_ECHEVRONANIM;

typedef struct
{
    SGUBRPROTOCOL_SRGB sRGB;
    const uint8_t* u8Lights;
    uint8_t u8LightCount;
} SGUBRPROTOCOL_SUpdateLightArg;

typedef struct
{
    uint32_t u32MaximumTimeMS;
} SGUBRPROTOCOL_SKeepAliveArg;

typedef struct
{
    SGUBRPROTOCOL_ECHEVRONANIM eChevronAnim;
} SGUBRPROTOCOL_SChevronsLightningArg;

typedef void(*fnUpdateLight)(const SGUBRPROTOCOL_SUpdateLightArg* psArg);
typedef void(*fnTurnOff)(void);
typedef void(*fnKeepAlive)(const SGUBRPROTOCOL_SKeepAliveArg* psKeepAliveArg);
typedef void(*fnChevronsLightning)(const SGUBRPROTOCOL_SChevronsLightningArg* psChevronLightningArg);

typedef void(*fnGotoFactory)();

typedef void(*fnGotoOTAMode)(void);

typedef struct
{
    fnKeepAlive fnKeepAliveHandler;
    fnTurnOff fnTurnOffHandler;

    fnUpdateLight fnUpdateLightHandler;
    fnChevronsLightning fnChevronsLightningHandler;

    fnGotoFactory fnGotoFactoryHandler;

    fnGotoOTAMode fnGotoOTAModeHandler;
} SGUBRPROTOCOL_SConfig;

typedef struct
{
    const SGUBRPROTOCOL_SConfig* psConfig;
} SGUBRPROTOCOL_SHandle;

void SGUBRPROTOCOL_Init(SGUBRPROTOCOL_SHandle* psHandle, const SGUBRPROTOCOL_SConfig* psConfig);

bool SGUBRPROTOCOL_Decode(const SGUBRPROTOCOL_SHandle* psHandle, const uint8_t* u8Datas, uint16_t u16Length);

// Encode keep alive command.
uint32_t SGUBRPROTOCOL_EncKeepAlive(uint8_t* u8Dst, uint16_t u16MaxLen, const SGUBRPROTOCOL_SKeepAliveArg* psArg);
uint32_t SGUBRPROTOCOL_EncTurnOff(uint8_t* u8Dst, uint16_t u16MaxLen);
uint32_t SGUBRPROTOCOL_EncUpdateLight(uint8_t* u8Dst, uint16_t u16MaxLen, const SGUBRPROTOCOL_SUpdateLightArg* psArg);
uint32_t SGUBRPROTOCOL_EncChevronLightning(uint8_t* u8Dst, uint16_t u16MaxLen, const SGUBRPROTOCOL_SChevronsLightningArg* psArg);
uint32_t SGUBRPROTOCOL_EncGotoFactory(uint8_t* u8Dst, uint16_t u16MaxLen);
uint32_t SGUBRPROTOCOL_EncGotoOTAMode(uint8_t* u8Dst, uint16_t u16MaxLen);

#ifdef __cplusplus
}
#endif

#endif