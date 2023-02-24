#ifndef _SOUNDFX_H_
#define _SOUNDFX_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    SOUNDFX_EFILE_1_beginroll_mp3 = 0,
    SOUNDFX_EFILE_2_chevlck_wav,
    SOUNDFX_EFILE_3_chevlck2_wav,
    SOUNDFX_EFILE_4_gateclos_mp3,
    SOUNDFX_EFILE_5_gateopen_mp3,
    SOUNDFX_EFILE_6_lggroll_mp3,
    SOUNDFX_EFILE_7_lockfail_wav,
    SOUNDFX_EFILE_8_music1_mp3,
    SOUNDFX_EFILE_9_music2_mp3,
    SOUNDFX_EFILE_10_wormhole_wav,

    SOUNDFX_EFILE_Count
} SOUNDFX_EFILE;

typedef struct 
{
    const char* szName;
    uint32_t u32TimeMS;
    const char* szDesc;
} SOUNDFX_SFile;

#define SOUNDFX_SFILE_INIT(_szName, _u32TimeMS, _szDesc) { .szName = _szName, .u32TimeMS = _u32TimeMS, .szDesc = _szDesc }

void SOUNDFX_Init();

void SOUNDFX_ActivateGate();

void SOUNDFX_StartRollingSound();

void SOUNDFX_EngageChevron();

void SOUNDFX_ChevronLock();

void SOUNDFX_WormholeOpen();

void SOUNDFX_WormholeIdling();

void SOUNDFX_WormholeClose();

void SOUNDFX_Fail();

const SOUNDFX_SFile* SOUNDFX_GetFile(SOUNDFX_EFILE eFile);

void SOUNDFX_PlayFile(SOUNDFX_EFILE eFile, bool bIsRepeat);

void SOUNDFX_Stop();

#endif