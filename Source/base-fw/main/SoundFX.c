#include "SoundFX.h"
#include "GPIO.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static SOUNDFX_SFile m_sFiles[] =
{
    [SOUNDFX_EFILE_1_beginroll_mp3] = SOUNDFX_SFILE_INIT("1_beginroll.mp3", ""),
    [SOUNDFX_EFILE_2_chevlck_wav]   = SOUNDFX_SFILE_INIT("2_chevlck.wav", ""),
    [SOUNDFX_EFILE_3_chevlck2_wav]  = SOUNDFX_SFILE_INIT("3_chevlck2.wav", ""),
    [SOUNDFX_EFILE_4_gateclos_mp3]  = SOUNDFX_SFILE_INIT("4_gateclos.mp3", ""),
    [SOUNDFX_EFILE_5_gateopen_mp3]  = SOUNDFX_SFILE_INIT("5_gateopen.mp3", ""),
    [SOUNDFX_EFILE_6_lggroll_mp3]   = SOUNDFX_SFILE_INIT("6_lggroll.mp3", ""),
    [SOUNDFX_EFILE_7_lockfail_wav]  = SOUNDFX_SFILE_INIT("7_lockfail.wav", ""),
    [SOUNDFX_EFILE_8_music1_mp3]    = SOUNDFX_SFILE_INIT("8_music1.mp3", ""),
    [SOUNDFX_EFILE_9_music2_mp3]    = SOUNDFX_SFILE_INIT("9_music2.mp3", ""),
};

_Static_assert((sizeof(m_sFiles)/sizeof(m_sFiles[0])) == SOUNDFX_EFILE_Count, "coucou");

void SOUNDFX_Init()
{
    SOUNDFX_Stop();
}

void SOUNDFX_ActivateGate()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_1_beginroll_mp3);
}

void SOUNDFX_StartRollingSound()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_6_lggroll_mp3);
}

void SOUNDFX_EngageChevron()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_3_chevlck2_wav);
}

void SOUNDFX_ChevronLock()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_2_chevlck_wav);
}

void SOUNDFX_Fail()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_7_lockfail_wav);
}

void SOUNDFX_WormholeOpen()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_5_gateopen_mp3);
}

void SOUNDFX_WormholeClose()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_4_gateclos_mp3);
}

void SOUNDFX_PlayFile(SOUNDFX_EFILE eFile)
{
    const uint32_t u32Ix = (uint32_t)eFile + 1;
    GPIO_SendMp3PlayerCMD("AT+PLAYMODE=3\r\n");
    vTaskDelay(pdMS_TO_TICKS(10));  // Not sure how long it needs to take the command but it doesn't like being spammed
    char szBuff[64];
    sprintf(szBuff, "AT+PLAYNUM=%d\r\n", u32Ix);
    GPIO_SendMp3PlayerCMD(szBuff);
}

void SOUNDFX_Stop()
{
    // We don't know when it is playing and there is no explicit STOP command. It just toggle
    // so the trick is to play something then immediately put it on pause.
    GPIO_SendMp3PlayerCMD("AT+PLAYMODE=3\r\n");
    vTaskDelay(pdMS_TO_TICKS(10));
    GPIO_SendMp3PlayerCMD("AT+PLAYNUM=6\r\n");
    vTaskDelay(pdMS_TO_TICKS(10));
    GPIO_SendMp3PlayerCMD("AT+PLAY=PP\r\n");
}
