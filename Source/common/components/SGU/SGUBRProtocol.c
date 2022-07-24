#include "SGUBRProtocol.h"
#include <assert.h>
#include <string.h>

#define MAGIC0 0xBE
#define MAGIC1 0xEF
#define MAGIC_LENGTH 2

void SGUBRPROTOCOL_Init(SGUBRPROTOCOL_SHandle* psHandle, const SGUBRPROTOCOL_SConfig* psConfig)
{
    psHandle->psConfig = psConfig;
}

bool SGUBRPROTOCOL_Decode(const SGUBRPROTOCOL_SHandle* psHandle, const uint8_t* u8Datas, uint16_t u16Length)
{
    assert(psHandle->psConfig != NULL);
    if (u16Length < MAGIC_LENGTH + 1)
        return false;
    
    if (u8Datas[0] != MAGIC0 || u8Datas[1] != MAGIC1)
        return false;

    u8Datas += MAGIC_LENGTH; // Skip header
    u16Length -= MAGIC_LENGTH;
    const SGUBRPROTOCOL_ECMD eCmd = u8Datas[0];
    switch(eCmd)
    {
        case SGUBRPROTOCOL_ECMD_KeepAlive:
        {
            if (u16Length < 5)
                return false;
            const SGUBRPROTOCOL_SKeepAliveArg sKeepAlive =
            {
                .u32MaximumTimeMS = (u8Datas[1] << 24U) | (u8Datas[2] << 16U) | (u8Datas[3] << 8U) | u8Datas[4]
            };
            if (psHandle->psConfig->fnKeepAliveHandler != NULL)
                psHandle->psConfig->fnKeepAliveHandler(&sKeepAlive);

            return true;
        }
        case SGUBRPROTOCOL_ECMD_UpdateLight:
        {
            // CMD | R | G | B | LIGHT0 | LIGHT1 | ...
            if (u16Length < 5)
                return false;

            // Lights
            uint8_t u8LightCount = u16Length - 4;

            const SGUBRPROTOCOL_SUpdateLightArg sUpdateLightArg =
            {
                .sRGB = { .u8Red = u8Datas[1], .u8Green = u8Datas[2], .u8Blue = u8Datas[3] },
                .u8Lights = u8Datas+4,
                .u8LightCount = u8LightCount
            };

            if (psHandle->psConfig->fnUpdateLightHandler != NULL)
                psHandle->psConfig->fnUpdateLightHandler(&sUpdateLightArg);

            return true;
        }
        case SGUBRPROTOCOL_ECMD_TurnOff:
        {
            if (psHandle->psConfig->fnTurnOffHandler != NULL)
                psHandle->psConfig->fnTurnOffHandler();
            return true;
        }
        case SGUBRPROTOCOL_ECMD_GotoFactory:
        {
            if (psHandle->psConfig->fnGotoFactoryHandler != NULL)
                psHandle->psConfig->fnGotoFactoryHandler();
            return true;
        }
        case SGUBRPROTOCOL_ECMD_GotoOTAMode:
        {
            if (psHandle->psConfig->fnGotoOTAModeHandler != NULL)
                psHandle->psConfig->fnGotoOTAModeHandler();
            return true;
        }
        case SGUBRPROTOCOL_ECMD_ChevronsLightning:
        {
            // CMD | ANIM ...
            if (u16Length < 2)
                return false;

            const SGUBRPROTOCOL_ECHEVRONANIM eChevronAnim = (SGUBRPROTOCOL_ECHEVRONANIM)u8Datas[1];
            if (eChevronAnim >= SGUBRPROTOCOL_ECHEVRONANIM_Count)
                return false;

            const SGUBRPROTOCOL_SChevronsLightningArg sChevronsLightningArg =
            {
                .eChevronAnim = eChevronAnim
            };

            if (psHandle->psConfig->fnChevronsLightningHandler != NULL)
                psHandle->psConfig->fnChevronsLightningHandler(&sChevronsLightningArg);
            return true;
        }
    }
    return false;
}

uint32_t SGUBRPROTOCOL_EncKeepAlive(uint8_t* u8Dst, uint16_t u16MaxLen, const SGUBRPROTOCOL_SKeepAliveArg* psArg)
{
    const uint16_t u16ReqLength = MAGIC_LENGTH+5;
    if (u16MaxLen < u16ReqLength)
        return 0;

    u8Dst[0] = MAGIC0;
    u8Dst[1] = MAGIC1; 
    u8Dst[2] = (uint8_t)SGUBRPROTOCOL_ECMD_KeepAlive;
    u8Dst[3] = (uint8_t)(psArg->u32MaximumTimeMS >> 24U);
    u8Dst[4] = (uint8_t)(psArg->u32MaximumTimeMS >> 16U);
    u8Dst[5] = (uint8_t)(psArg->u32MaximumTimeMS >> 8U);
    u8Dst[6] = (uint8_t)(psArg->u32MaximumTimeMS);
    return u16ReqLength;
}

uint32_t SGUBRPROTOCOL_EncTurnOff(uint8_t* u8Dst, uint16_t u16MaxLen)
{
    const uint16_t u16ReqLength = MAGIC_LENGTH+1;
    if (u16MaxLen < u16ReqLength)
        return 0;

    u8Dst[0] = MAGIC0;
    u8Dst[1] = MAGIC1; 
    u8Dst[2] = (uint8_t)SGUBRPROTOCOL_ECMD_TurnOff;
    return u16ReqLength;
}

uint32_t SGUBRPROTOCOL_EncGotoFactory(uint8_t* u8Dst, uint16_t u16MaxLen)
{
    const uint16_t u16ReqLength = MAGIC_LENGTH+1;
    if (u16MaxLen < u16ReqLength)
        return 0;

    u8Dst[0] = MAGIC0;
    u8Dst[1] = MAGIC1; 
    u8Dst[2] = (uint8_t)SGUBRPROTOCOL_ECMD_GotoFactory;
    return u16ReqLength;
}

uint32_t SGUBRPROTOCOL_EncGotoOTAMode(uint8_t* u8Dst, uint16_t u16MaxLen)
{
    const uint16_t u16ReqLength = MAGIC_LENGTH+1;
    if (u16MaxLen < u16ReqLength)
        return 0;

    u8Dst[0] = MAGIC0;
    u8Dst[1] = MAGIC1; 
    u8Dst[2] = (uint8_t)SGUBRPROTOCOL_ECMD_GotoOTAMode;
    return u16ReqLength;
}

uint32_t SGUBRPROTOCOL_EncUpdateLight(uint8_t* u8Dst, uint16_t u16MaxLen, const SGUBRPROTOCOL_SUpdateLightArg* psArg)
{
    const uint16_t u16ReqLength = MAGIC_LENGTH + /*Cmd*/1 + /*R|G|B*/3 + /*Light indexes...*/psArg->u8LightCount;
    if (u16MaxLen < u16ReqLength)
        return 0;

    u8Dst[0] = MAGIC0;
    u8Dst[1] = MAGIC1; 
    u8Dst[2] = (uint8_t)SGUBRPROTOCOL_ECMD_UpdateLight;
    u8Dst[3] = psArg->sRGB.u8Red;
    u8Dst[4] = psArg->sRGB.u8Green;
    u8Dst[5] = psArg->sRGB.u8Blue;
    memcpy(u8Dst+6, psArg->u8Lights, psArg->u8LightCount);
    return u16ReqLength;
}

uint32_t SGUBRPROTOCOL_EncChevronLightning(uint8_t* u8Dst, uint16_t u16MaxLen, const SGUBRPROTOCOL_SChevronsLightningArg* psArg)
{
    const uint16_t u16ReqLength = MAGIC_LENGTH + /*Cmd*/1 + /*Anim Type*/1;
    if (u16MaxLen < u16ReqLength)
        return 0;

    u8Dst[0] = MAGIC0;
    u8Dst[1] = MAGIC1; 
    u8Dst[2] = (uint8_t)SGUBRPROTOCOL_ECMD_ChevronsLightning;
    u8Dst[3] = (uint8_t)psArg->eChevronAnim;
    return u16ReqLength;
}