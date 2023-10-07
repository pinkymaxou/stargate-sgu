#include "WebAPI.h"
#include "cJSON.h"
#include "EmbeddedFiles.h"
#include "SoundFX.h"
#include "GateControl.h"
#include "Settings.h"
#include "HelperMacro.h"
#include "GPIO.h"
#include <stdio.h>
#include <time.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_netif_types.h"
#include "Main.h"
#include "esp_ota_ops.h"
#include "ApiURL.h"

#define TAG "WebAPI"

/*! @brief this variable is set by linker script, don't rename it. It contains app image informations. */
extern const esp_app_desc_t esp_app_desc;

static void ToHexString(char *dstHexString, const uint8_t* data, uint8_t len);
static char* GetSysInfo();
static char* GetStatusJSON();
static char* GetAllSounds();

#define HTTPSERVER_BUFFERSIZE (1024*10)
static uint8_t m_u8Buffers[HTTPSERVER_BUFFERSIZE];

esp_err_t WEBAPI_GetHandler(httpd_req_t *req)
{
    esp_err_t err = ESP_OK;

    char* pExportJSON = NULL;

    if (strcmp(req->uri, API_GETSETTINGSJSON_URI) == 0)
    {
        pExportJSON = NVSJSON_ExportJSON(&g_sSettingHandle);

        if (pExportJSON == NULL || httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to send data");
            goto ERROR;
        }
    }
    else if (strcmp(req->uri, API_GETSYSINFOJSON_URI) == 0)
    {
        pExportJSON = GetSysInfo();

        if (pExportJSON == NULL || httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to send data");
            goto ERROR;
        }
    }
    else if (strcmp(req->uri, API_GETALLSOUNDSJSON_URI) == 0)
    {
        pExportJSON = GetAllSounds();
        if (pExportJSON == NULL || httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to send data");
            goto ERROR;
        }
    }
    else if (strcmp(req->uri, API_GETSTATUSJSON_URI) == 0)
    {
        pExportJSON = GetStatusJSON();
        if (pExportJSON == NULL || httpd_resp_send_chunk(req, pExportJSON, strlen(pExportJSON)) != ESP_OK)
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to send data");
            goto ERROR;
        }
    }
    else
    {
        ESP_LOGE(TAG, "api_get_handler, url: %s", req->uri);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown request");
        goto ERROR;
    }
    goto END;
    ERROR:
    err = ESP_FAIL;
    END:
    if (pExportJSON != NULL)
        free(pExportJSON);
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    return err;
}

static char* GetSysInfo()
{
    cJSON* pRoot = NULL;

    char buff[100];
    pRoot = cJSON_CreateObject();
    if (pRoot == NULL)
    {
        goto ERROR;
    }
    cJSON* pEntries = cJSON_AddArrayToObject(pRoot, "infos");

    // Firmware
    cJSON* pEntryJSON1 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON1, "name", cJSON_CreateString("Firmware"));
    cJSON_AddItemToObject(pEntryJSON1, "value", cJSON_CreateString(esp_app_desc.version));
    cJSON_AddItemToArray(pEntries, pEntryJSON1);

    // Compile Time
    cJSON* pEntryJSON2 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON2, "name", cJSON_CreateString("Compile Time"));
    sprintf(buff, "%s %s", /*0*/esp_app_desc.date, /*0*/esp_app_desc.time);
    cJSON_AddItemToObject(pEntryJSON2, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON2);

    // SHA256
    cJSON* pEntryJSON3 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON3, "name", cJSON_CreateString("SHA256"));
    char elfSHA256[sizeof(esp_app_desc.app_elf_sha256)*2 + 1] = {0,};
    ToHexString(elfSHA256, esp_app_desc.app_elf_sha256, sizeof(esp_app_desc.app_elf_sha256));
    cJSON_AddItemToObject(pEntryJSON3, "value", cJSON_CreateString(elfSHA256));
    cJSON_AddItemToArray(pEntries, pEntryJSON3);

    // IDF
    cJSON* pEntryJSON4 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON4, "name", cJSON_CreateString("IDF"));
    cJSON_AddItemToObject(pEntryJSON4, "value", cJSON_CreateString(esp_app_desc.idf_ver));
    cJSON_AddItemToArray(pEntries, pEntryJSON4);

    // WiFi-AP
    cJSON* pEntryJSON5 = cJSON_CreateObject();
    uint8_t u8Macs[6];
    cJSON_AddItemToObject(pEntryJSON5, "name", cJSON_CreateString("WiFi.AP"));
    esp_read_mac(u8Macs, ESP_MAC_WIFI_SOFTAP);
    sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X", /*0*/u8Macs[0], /*1*/u8Macs[1], /*2*/u8Macs[2], /*3*/u8Macs[3], /*4*/u8Macs[4], /*5*/u8Macs[5]);
    cJSON_AddItemToObject(pEntryJSON5, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON5);

    // WiFi-STA
    cJSON* pEntryJSON6 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON6, "name", cJSON_CreateString("WiFi.STA"));
    esp_read_mac(u8Macs, ESP_MAC_WIFI_STA);
    sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X", /*0*/u8Macs[0], /*1*/u8Macs[1], /*2*/u8Macs[2], /*3*/u8Macs[3], /*4*/u8Macs[4], /*5*/u8Macs[5]);
    cJSON_AddItemToObject(pEntryJSON6, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON6);

    // WiFi-BT
    cJSON* pEntryJSON7 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON7, "name", cJSON_CreateString("WiFi.BT"));
    esp_read_mac(u8Macs, ESP_MAC_BT);
    sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X", /*0*/u8Macs[0], /*1*/u8Macs[1], /*2*/u8Macs[2], /*3*/u8Macs[3], /*4*/u8Macs[4], /*5*/u8Macs[5]);
    cJSON_AddItemToObject(pEntryJSON7, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON7);

    // Memory
    cJSON* pEntryJSON8 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON8, "name", cJSON_CreateString("Memory"));
    const int freeSize = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    const int totalSize = heap_caps_get_total_size(MALLOC_CAP_8BIT);

    sprintf(buff, "%d / %d", /*0*/freeSize, /*1*/totalSize);
    cJSON_AddItemToObject(pEntryJSON8, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON8);

    // WiFi-station (IP address)
    cJSON* pEntryJSON9 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON9, "name", cJSON_CreateString("WiFi (STA)"));
    esp_netif_ip_info_t wifiIpSta = {0};
    MAIN_GetWiFiSTAIP(&wifiIpSta);
    sprintf(buff, IPSTR, IP2STR(&wifiIpSta.ip));
    cJSON_AddItemToObject(pEntryJSON9, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON9);

    esp_ip6_addr_t if_ip6[CONFIG_LWIP_IPV6_NUM_ADDRESSES] = {0};
    const int32_t s32IPv6Count = MAIN_GetWiFiSTAIPv6(if_ip6);
    for(int i = 0; i < HELPERMACRO_MIN(s32IPv6Count, 2); i++)
    {
        char ipv6String[45+1] = {0,};
        snprintf(ipv6String, sizeof(ipv6String)-1, IPV6STR, IPV62STR(if_ip6[i]));

        cJSON* pEntryJSONIPv6 = cJSON_CreateObject();
        cJSON_AddItemToObject(pEntryJSONIPv6, "name", cJSON_CreateString("WiFi (STA) IPv6"));
        cJSON_AddItemToObject(pEntryJSONIPv6, "value", cJSON_CreateString(ipv6String));
        cJSON_AddItemToArray(pEntries, pEntryJSONIPv6);
    }

    // WiFi-Soft AP (IP address)
    cJSON* pEntryJSON10 = cJSON_CreateObject();
    cJSON_AddItemToObject(pEntryJSON10, "name", cJSON_CreateString("WiFi (Soft-AP)"));
    esp_netif_ip_info_t wifiIpSoftAP = {0};
    MAIN_GetWiFiSoftAPIP(&wifiIpSoftAP);
    sprintf(buff, IPSTR, IP2STR(&wifiIpSoftAP.ip));
    cJSON_AddItemToObject(pEntryJSON10, "value", cJSON_CreateString(buff));
    cJSON_AddItemToArray(pEntries, pEntryJSON10);


    char* pStr =  cJSON_PrintUnformatted(pRoot);
    cJSON_Delete(pRoot);
    return pStr;
    ERROR:
    cJSON_Delete(pRoot);
    return NULL;
}

static char* GetStatusJSON()
{
    cJSON* pRoot = NULL;
    pRoot = cJSON_CreateObject();
    if (pRoot == NULL)
        goto ERROR;

    cJSON* pStatusEntry = cJSON_CreateObject();

    GATECONTROL_SState sState;
    GATECONTROL_GetState(&sState);

    cJSON_AddItemToObject(pStatusEntry, "text", cJSON_CreateString(sState.szStatusText));
    cJSON_AddItemToObject(pStatusEntry, "cancel_request", cJSON_CreateBool(sState.bIsCancelRequested));

    cJSON_AddItemToObject(pStatusEntry, "error_text", cJSON_CreateString(sState.szLastError));
    cJSON_AddItemToObject(pStatusEntry, "is_error", cJSON_CreateBool(sState.bHasLastError));

    time_t now = 0;
    struct tm timeinfo = { 0 };
    time(&now);
    localtime_r(&now, &timeinfo);
    cJSON_AddItemToObject(pStatusEntry, "time_hour", cJSON_CreateNumber(timeinfo.tm_hour));
    cJSON_AddItemToObject(pStatusEntry, "time_min", cJSON_CreateNumber(timeinfo.tm_min));

    cJSON_AddItemToObject(pRoot, "status", pStatusEntry);

    char* pStr =  cJSON_PrintUnformatted(pRoot);
    cJSON_Delete(pRoot);
    return pStr;
    ERROR:
    cJSON_Delete(pRoot);
    return NULL;
}

static char* GetAllSounds()
{
    cJSON* pRoot = NULL;
    pRoot = cJSON_CreateObject();
    if (pRoot == NULL)
        goto ERROR;

    cJSON* pEntries = cJSON_AddArrayToObject(pRoot, "files");

    for(int i = 0; i < 10; i++)
    {
        const SOUNDFX_SFile* pFile = SOUNDFX_GetFile((SOUNDFX_EFILE)i);

        cJSON* pNewFile = cJSON_CreateObject();
        cJSON_AddItemToObject(pNewFile, "name", cJSON_CreateString(pFile->szName));
        cJSON_AddItemToObject(pNewFile, "desc", cJSON_CreateString(pFile->szDesc));
        cJSON_AddItemToArray(pEntries, pNewFile);
    }

    char* pStr = cJSON_PrintUnformatted(pRoot);
    cJSON_Delete(pRoot);
    return pStr;
    ERROR:
    cJSON_Delete(pRoot);
    return NULL;
}

esp_err_t WEBAPI_ActionHandler(httpd_req_t *req)
{
    cJSON* root = NULL;

    int n = 0;
    while(1)
    {
        int reqN = httpd_req_recv(req, (char*)m_u8Buffers + n, HTTPSERVER_BUFFERSIZE - n - 1);
        if (reqN <= 0)
        {
            ESP_LOGI(TAG, "api_post_handler, test: %d, reqN: %d", (int)n, (int)reqN);
            break;
        }
        n += reqN;
    }
    m_u8Buffers[n] = '\0';

    ESP_LOGI(TAG, "Receiving content: %s", m_u8Buffers);

    ESP_LOGI(TAG, "file_post_handler, url: %s", req->uri);

    if (strcmp(req->uri, ACTION_POST_GOHOME) == 0)
        GATECONTROL_DoAction(GATECONTROL_EMODE_GoHome, NULL);
    else if (strcmp(req->uri, ACTION_POST_RELEASECLAMP) == 0)
        GATECONTROL_DoAction(GATECONTROL_EMODE_ManualReleaseClamp, NULL);
    else if (strcmp(req->uri, ACTION_POST_LOCKCLAMP) == 0)
        GATECONTROL_DoAction(GATECONTROL_EMODE_ManualLockClamp, NULL);
    else if (strcmp(req->uri, ACTION_POST_STOP) == 0)
        GATECONTROL_DoAction(GATECONTROL_EMODE_Stop, NULL);
    else if (strcmp(req->uri, ACTION_POST_AUTOCALIBRATE) == 0)
        GATECONTROL_DoAction(GATECONTROL_EMODE_AutoCalibrate, NULL);
    else if (strcmp(req->uri, ACTION_POST_DIAL) == 0)
    {
        // Decode JSON
        root = cJSON_Parse((const char*)m_u8Buffers);
        if (root == NULL || !cJSON_IsObject(root))
        {
            ESP_LOGE(TAG, "cJSON_IsObject");
            goto ERROR;
        }
        GATECONTROL_UModeArg uModeArg;

        const cJSON *itemArray;
        uModeArg.sDialArg.u8SymbolCount = 0;
        cJSON* cJsonSymbols = cJSON_GetObjectItem(root, "symbols");
        if (cJsonSymbols == NULL || !cJSON_IsArray(cJsonSymbols))
        {
            ESP_LOGE(TAG, "symbol");
            goto ERROR;
        }

        cJSON_ArrayForEach(itemArray, cJsonSymbols)
        {
            if (!cJSON_IsNumber(itemArray))
                goto ERROR;
            uModeArg.sDialArg.u8Symbols[uModeArg.sDialArg.u8SymbolCount] = itemArray->valueint;
            uModeArg.sDialArg.u8SymbolCount++;
        }

        cJSON* cJsonWormhole = cJSON_GetObjectItem(root, "wormhole");
        if (cJsonWormhole)
            uModeArg.sDialArg.eWormholeType = (WORMHOLE_ETYPE)cJsonWormhole->valueint;
        else
            uModeArg.sDialArg.eWormholeType = (WORMHOLE_ETYPE)NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WormholeType);

        if (!GATECONTROL_DoAction(GATECONTROL_EMODE_Dial, &uModeArg))
            goto ERROR;
    }
    else if (strcmp(req->uri, ACTION_POST_MANUALRAMPLIGHT) == 0)
    {
        // Decode JSON
        root = cJSON_Parse((const char*)m_u8Buffers);
        if (root == NULL)
            goto ERROR;
        cJSON* pKeyJSON = cJSON_GetObjectItem(root, "light_perc");
        if (pKeyJSON == NULL || !cJSON_IsNumber(pKeyJSON))
        {
            ESP_LOGE(TAG, "Cannot find JSON key element");
            goto ERROR;
        }

        GPIO_SetRampLightPerc((float)pKeyJSON->valueint / 100.0f);
    }
    else if (strcmp(req->uri, ACTION_POST_RAMPLIGHTON) == 0)
        GATECONTROL_AnimRampLight(true);
    else if (strcmp(req->uri, ACTION_POST_RAMPLIGHTOFF) == 0)
        GATECONTROL_AnimRampLight(false);
    else if (strcmp(req->uri, ACTION_POST_RINGTURNOFF) == 0)
    {
        ESP_LOGI(TAG, "GateControl ring turn off");
        SGUBRCOMM_TurnOff(&g_sSGUBRCOMMHandle);
    }
    else if (strcmp(req->uri, ACTION_POST_ACTIVATEWORMHOLE) == 0)
    {
        // Decode JSON
        root = cJSON_Parse((const char*)m_u8Buffers);
        if (root == NULL)
            goto ERROR;
        GATECONTROL_UModeArg uModeArg;
        cJSON* cJsonWormhole = cJSON_GetObjectItem(root, "wormhole");
        if (cJsonWormhole)
            uModeArg.sManualWormhole.eWormholeType = (WORMHOLE_ETYPE)cJsonWormhole->valueint;
        else
            uModeArg.sManualWormhole.eWormholeType = (WORMHOLE_ETYPE)NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WormholeType);

        GATECONTROL_DoAction(GATECONTROL_EMODE_ManualWormhole, &uModeArg);
    }
    else if (strcmp(req->uri, ACTION_POST_ACTIVATECLOCKMODE) == 0)
        GATECONTROL_DoAction(GATECONTROL_EMODE_ActiveClock, NULL);
    else if (strcmp(req->uri, ACTION_POST_RINGCHEVRONERROR) == 0)
    {
        ESP_LOGI(TAG, "GateControl ring chevron error");
        SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_ErrorToWhite);
    }
    else if (strcmp(req->uri, ACTION_POST_RINGCHEVRONFADEIN) == 0)
    {
        ESP_LOGI(TAG, "GateControl ring chevron fade-in");
        SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_FadeIn);
    }
    else if (strcmp(req->uri, ACTION_POST_RINGCHEVRONFADEOUT) == 0)
    {
        ESP_LOGI(TAG, "GateControl ring chevron fade-out");
        SGUBRCOMM_ChevronLightning(&g_sSGUBRCOMMHandle, SGUBRPROTOCOL_ECHEVRONANIM_FadeOut);
    }
    else if (strcmp(req->uri, ACTION_POST_RINGGOTOFACTORY) == 0)
    {
        ESP_LOGI(TAG, "GateControl ring goto factory");
        SGUBRCOMM_GotoFactory(&g_sSGUBRCOMMHandle);
    }
    else if (strcmp(req->uri, ACTION_POST_REBOOT) == 0)
        MAIN_RequestReboot();
    else
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unknown request");

    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    if (root != NULL)
        cJSON_Delete(root);
    return ESP_OK;
    ERROR:
    ESP_LOGE(TAG, "Invalid request");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad request");
    if (root != NULL)
        cJSON_Delete(root);
    return ESP_FAIL;
}

esp_err_t WEBAPI_PostHandler(httpd_req_t *req)
{
    esp_err_t err = ESP_OK;

    int n = 0;
    while(1)
    {
        int reqN = httpd_req_recv(req, (char*)m_u8Buffers + n, HTTPSERVER_BUFFERSIZE - n - 1);
        if (reqN <= 0)
        {
            ESP_LOGI(TAG, "api_post_handler, test: %d, reqN: %d", (int)n, (int)reqN);
            break;
        }
        n += reqN;
    }
    m_u8Buffers[n] = '\0';

    ESP_LOGI(TAG, "api_post_handler, url: %s", req->uri);
    if (strcmp(req->uri, API_POSTSETTINGSJSON_URI) == 0)
    {
        if (!NVSJSON_ImportJSON(&g_sSettingHandle, (const char*)m_u8Buffers))
        {
            ESP_LOGE(TAG, "Unable to import JSON");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unknown request");
            goto ERROR;
        }
    }
    else
    {
        ESP_LOGE(TAG, "api_post_handler, url: %s", req->uri);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown request");
        goto ERROR;
    }
    goto END;
    ERROR:
    err = ESP_FAIL;
    END:
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    return err;
}

static void ToHexString(char *dstHexString, const uint8_t* data, uint8_t len)
{
    for (uint32_t i = 0; i < len; i++)
        sprintf(dstHexString + (i * 2), "%02X", data[i]);
}
