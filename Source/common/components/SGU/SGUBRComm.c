#include "SGUBRComm.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "esp_log.h"
#include <string.h>

#define SGUBRCOMM_BASESERVER_PORT 5000
#define SGUBRCOMM_RINGSERVER_PORT 5001

#define SGUBRCOMM_TAKE(pHandle) xSemaphoreTake(pHandle->sSemaphoreHandle, portMAX_DELAY);
#define SGUBRCOMM_GIVE(pHandle) xSemaphoreGive(pHandle->sSemaphoreHandle);

#define TAG "SGUBRCOMM"

// Control homing and encoder.
static bool SendRingData(SGUBRCOMM_SHandle* pHandle, const uint8_t* u8Datas, uint32_t u32Len);

void SGUBRCOMM_Init(SGUBRCOMM_SHandle* pHandle, const SGUBRCOMM_SSetting* psSetting)
{
    pHandle->psSetting = psSetting;

    pHandle->sSemaphoreHandle = xSemaphoreCreateMutexStatic(&pHandle->sSemaphoreCreateMutex);
    configASSERT( pHandle->sSemaphoreHandle );

    memset(pHandle->u8Buffers, 0, sizeof(pHandle->u8Buffers));
    pHandle->sock = 0;
    
    pHandle->lAutoOffTicks = 0;

    SGUBRPROTOCOL_Init(&pHandle->sSGUBRProtHandle, psSetting->psSGUBRProtocolConfig);
}

void SGUBRCOMM_Start(SGUBRCOMM_SHandle* pHandle)
{
    SGUBRCOMM_TAKE(pHandle);
    uint16_t u16Port = 0;
    if (pHandle->psSetting->eMode == SGUBRCOMM_EMODE_Base)
        u16Port = SGUBRCOMM_BASESERVER_PORT;
    else if (pHandle->psSetting->eMode == SGUBRCOMM_EMODE_Ring)
        u16Port = SGUBRCOMM_RINGSERVER_PORT;

    struct sockaddr_in dest_addr_ip4;
    dest_addr_ip4.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4.sin_family = AF_INET;
    dest_addr_ip4.sin_port = htons(u16Port);

    pHandle->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (pHandle->sock < 0) 
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        goto END;
    }
    ESP_LOGI(TAG, "Socket created");

    int err = bind(pHandle->sock, (struct sockaddr *)&dest_addr_ip4, sizeof(dest_addr_ip4));
    if (err < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        goto END;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", u16Port);

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10;
    setsockopt(pHandle->sock, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
    END:
    SGUBRCOMM_GIVE(pHandle);
}

void SGUBRCOMM_Process(SGUBRCOMM_SHandle* pHandle)
{
    SGUBRCOMM_TAKE(pHandle);

    struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
    socklen_t socklen = sizeof(source_addr);
    int len = recvfrom(pHandle->sock, pHandle->u8Buffers, sizeof(pHandle->u8Buffers) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

    SGUBRCOMM_GIVE(pHandle);
    
    if (len > 0)
    {
        ESP_LOGI(TAG, "Receiving data, len: %d", len);
        if (!SGUBRPROTOCOL_Decode(&pHandle->sSGUBRProtHandle, pHandle->u8Buffers, len))
        {
            ESP_LOGE(TAG, "Unable to decode message");
        }
    }
}

static bool SendRingData(SGUBRCOMM_SHandle* pHandle, const uint8_t* u8Datas, uint32_t u32Len)
{
    SGUBRCOMM_TAKE(pHandle);
    struct sockaddr_in sSockAddrIn;
    sSockAddrIn.sin_family    = AF_INET;
    sSockAddrIn.sin_port = htons(SGUBRCOMM_RINGSERVER_PORT);
    sSockAddrIn.sin_addr.s_addr = inet_addr(pHandle->psSetting->dstRingAddress);
    
    ESP_LOGI(TAG, "Sending data to: %s:%d", pHandle->psSetting->dstRingAddress, SGUBRCOMM_RINGSERVER_PORT);

    int err = sendto(pHandle->sock, u8Datas, u32Len, 0, (struct sockaddr *)&sSockAddrIn, sizeof(sSockAddrIn));
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        SGUBRCOMM_GIVE(pHandle);
        return false;
    }
    SGUBRCOMM_GIVE(pHandle);
    return true;
}

void SGUBRCOMM_ChevronLightning(SGUBRCOMM_SHandle* pHandle, SGUBRPROTOCOL_ECHEVRONANIM eChevronAnim)
{
    static uint8_t buffer[20] = { 0 };

    const SGUBRPROTOCOL_SChevronsLightningArg arg = 
    {
        .eChevronAnim = eChevronAnim
    };

    const int n = SGUBRPROTOCOL_EncChevronLightning(buffer, sizeof(buffer), &arg);
    SendRingData(pHandle, buffer, n);
    ESP_LOGI(TAG, "SGUCOMM_ChevronLightning, n: %d", /*0*/n);
}

void SGUBRCOMM_LightUpLED(SGUBRCOMM_SHandle* pHandle, uint8_t u8Red, uint8_t u8Green, uint8_t u8Blue, uint8_t u8LedIndex)
{
    uint8_t buffer[20] = { 0 };
    
    const uint8_t u8Lights[] = { u8LedIndex };
    const SGUBRPROTOCOL_SUpdateLightArg arg = 
    {
        .sRGB = { .u8Red = u8Red, .u8Green = u8Green, .u8Blue = u8Blue },
        .u8Lights = u8Lights,
        .u8LightCount = sizeof(u8Lights) / sizeof(uint8_t)
    };

    const int n = SGUBRPROTOCOL_EncUpdateLight(buffer, sizeof(buffer), &arg);   
    SendRingData(pHandle, buffer, n);
    ESP_LOGI(TAG, "SGUCOMM_LightUpLED, len: %d, r: %d, g: %d, b: %d, index: %d", /*0*/n, /*1*/u8Red, /*2*/u8Green, /*3*/u8Blue, /*4*/u8LedIndex);    
}

void SGUBRCOMM_TurnOff(SGUBRCOMM_SHandle* pHandle)
{
    uint8_t buffer[20] = { 0 };
    const int n = SGUBRPROTOCOL_EncTurnOff(buffer, sizeof(buffer));   
    SendRingData(pHandle, buffer, n);
    ESP_LOGI(TAG, "SGUBRCOMM_TurnOff");
}

void SGUBRCOMM_GotoFactory(SGUBRCOMM_SHandle* pHandle)
{
    uint8_t buffer[20] = { 0 };
    const int n = SGUBRPROTOCOL_EncGotoFactory(buffer, sizeof(buffer));   
    SendRingData(pHandle, buffer, n);
    ESP_LOGI(TAG, "SGUBRCOMM_GotoFactory");
}