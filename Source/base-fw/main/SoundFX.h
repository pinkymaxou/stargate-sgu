#ifndef _SOUNDFX_H_
#define _SOUNDFX_H_

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

    SOUNDFX_EFILE_Count
} SOUNDFX_EFILE;

typedef struct 
{
    const char* szName;
    const char* szDesc;
} SOUNDFX_SFile;

#define SOUNDFX_SFILE_INIT(_szName, _szDesc) { .szName = _szName, .szDesc = _szDesc }

void SOUNDFX_Init();

void SOUNDFX_ActivateGate();

void SOUNDFX_StartRollingSound();

void SOUNDFX_EngageChevron();

void SOUNDFX_ChevronLock();

void SOUNDFX_WormholeOpen();

void SOUNDFX_WormholeClose();

void SOUNDFX_Fail();

void SOUNDFX_PlayFile(SOUNDFX_EFILE eFile);

void SOUNDFX_Stop();

#endif