#include "SoundFX.h"
#include "GPIO.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Settings.h"

static SOUNDFX_SFile m_sFiles[] =
{
    [SOUNDFX_EFILE_1_beginroll_mp3]  = SOUNDFX_SFILE_INIT("1_beginroll.mp3", 1250, "Gate activation before dialing"),
    [SOUNDFX_EFILE_2_chevlck_wav]    = SOUNDFX_SFILE_INIT("2_chevlck.wav",    750, "Chevron lock #1"),
    [SOUNDFX_EFILE_3_chevlck2_wav]   = SOUNDFX_SFILE_INIT("3_chevlck2.wav",  1200, "Chevron lock #2 (light on)"),
    [SOUNDFX_EFILE_4_gateclos_mp3]   = SOUNDFX_SFILE_INIT("4_gateclos.mp3",  2500, "Wormhole is closing"),
    [SOUNDFX_EFILE_5_gateopen_mp3]   = SOUNDFX_SFILE_INIT("5_gateopen.mp3",  5000, "Wormhole is opening"),
    [SOUNDFX_EFILE_6_lggroll_mp3]    = SOUNDFX_SFILE_INIT("6_lggroll.mp3",  26500, "Stargate is spinning"),
    [SOUNDFX_EFILE_7_lockfail_wav]   = SOUNDFX_SFILE_INIT("7_lockfail.wav",  1000, "Chevron lock failed"),
    [SOUNDFX_EFILE_8_music1_mp3]     = SOUNDFX_SFILE_INIT("8_music1.mp3",  346000, "Countdown to destiny"),
    [SOUNDFX_EFILE_9_music2_mp3]     = SOUNDFX_SFILE_INIT("9_music2.mp3",  173000, "Ending music"),
    [SOUNDFX_EFILE_10_wormhole_wav]  = SOUNDFX_SFILE_INIT("10_wormhole.wav", 5319, "Wormhole activated"),
};

_Static_assert((sizeof(m_sFiles)/sizeof(m_sFiles[0])) == SOUNDFX_EFILE_Count, "doesn't match");

void SOUNDFX_Init()
{
    // SOUNDFX_Stop();
}

const SOUNDFX_SFile* SOUNDFX_GetFile(SOUNDFX_EFILE eFile)
{
    return &m_sFiles[(int)eFile];
}

void SOUNDFX_ActivateGate()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_1_beginroll_mp3, false);
}

void SOUNDFX_StartRollingSound()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_6_lggroll_mp3, false);
}

void SOUNDFX_EngageChevron()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_3_chevlck2_wav, false);
}

void SOUNDFX_ChevronLock()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_2_chevlck_wav, false);
}

void SOUNDFX_Fail()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_7_lockfail_wav, false);
}

void SOUNDFX_WormholeOpen()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_5_gateopen_mp3, false);
}

void SOUNDFX_WormholeIdling()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_10_wormhole_wav, true);
}

void SOUNDFX_WormholeClose()
{
    SOUNDFX_PlayFile(SOUNDFX_EFILE_4_gateclos_mp3, false);
}

void SOUNDFX_PlayFile(SOUNDFX_EFILE eFile, bool bIsRepeat)
{
    char szBuff[64];
    sprintf(szBuff, "AT+VOL=%d\r\n", (int)NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_Mp3PlayerVolume));
    GPIO_SendMp3PlayerCMD(szBuff);
    vTaskDelay(pdMS_TO_TICKS(25));

    // Play once
    if (bIsRepeat)
        GPIO_SendMp3PlayerCMD("AT+PLAYMODE=1\r\n");
    else
        GPIO_SendMp3PlayerCMD("AT+PLAYMODE=3\r\n");
    vTaskDelay(pdMS_TO_TICKS(25));  // Not sure how long it needs to take the command but it doesn't like being spammed

    // Play number
    const uint32_t u32Ix = (uint32_t)eFile + 1;
    sprintf(szBuff, "AT+PLAYNUM=%d\r\n", (int)u32Ix);
    GPIO_SendMp3PlayerCMD(szBuff);
}

void SOUNDFX_Stop()
{
    // We don't know when it is playing and there is no explicit STOP command. It just toggle
    // so the trick is to play something then immediately put it on pause.
    GPIO_SendMp3PlayerCMD("AT+PLAYMODE=3\r\n");
    vTaskDelay(pdMS_TO_TICKS(35));
    GPIO_SendMp3PlayerCMD("AT+PLAYNUM=6\r\n");
    vTaskDelay(pdMS_TO_TICKS(35));
    GPIO_SendMp3PlayerCMD("AT+PLAY=PP\r\n");
}
