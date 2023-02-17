#include "SoundFX.h"
#include "GPIO.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void SOUNDFX_Init()
{
    GPIO_SendMp3PlayerCMD("AT+PLAYMODE=3\r\n");
}

void SOUNDFX_ActivateGate()
{
    GPIO_SendMp3PlayerCMD("AT+PLAYNUM=1\r\n");
}

void SOUNDFX_StartRollingSound()
{
    GPIO_SendMp3PlayerCMD("AT+PLAYNUM=6\r\n");
}

void SOUNDFX_EngageChevron()
{
    GPIO_SendMp3PlayerCMD("AT+PLAYNUM=3\r\n");
}

void SOUNDFX_ChevronLock()
{
    GPIO_SendMp3PlayerCMD("AT+PLAYNUM=2\r\n");
}

void SOUNDFX_Fail()
{
    GPIO_SendMp3PlayerCMD("AT+PLAYNUM=7\r\n");
}

void SOUNDFX_WormholeOpen()
{
    GPIO_SendMp3PlayerCMD("AT+PLAYNUM=5\r\n");
}

void SOUNDFX_WormholeClose()
{
    GPIO_SendMp3PlayerCMD("AT+PLAYNUM=4\r\n");
}

void SOUNDFX_Stop()
{
    // We don't know when it is playing and there is no explicit STOP command. It just toggle
    // so the trick is to play something then immediately put it on pause.
    GPIO_SendMp3PlayerCMD("AT+PLAYNUM=6\r\n");
    vTaskDelay(pdMS_TO_TICKS(50));
    GPIO_SendMp3PlayerCMD("AT+PLAY=PP\r\n");
}
