/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include "lwip/err.h"
#include "lwip/sys.h"
#include "webserver/WebServer.h"
#include "lwip/apps/netbiosns.h"
#include "GPIO.h"
#include "FWConfig.h"
#include "Settings.h"
#include "Main.h"
#include "GateControl.h"
#include "GateStepper.h"
#include "SoundFX.h"
#include "SSD1306.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

static const char *TAG = "wifi";

SGUBRCOMM_SHandle g_sSGUBRCOMMHandle;
static SGUBRCOMM_SSetting m_sSetting = { .eMode = SGUBRCOMM_EMODE_Base };
static TickType_t m_xRebootRequestTicks = 0;
static char m_szSoftAPSSID[31] = {0,};

static esp_netif_t* m_pWifiSoftAP;
static esp_netif_t* m_pWifiSTA;

static void wifi_init_all(void);
static void wifi_init_softap(void);

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifistation_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static void PrintCurrentTime();

static void time_sync_notification_cb(struct timeval *tv);

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station %02x:%02x:%02x:%02x:%02x:%02x join, AID=%d",
                 event->mac[0], event->mac[1],event->mac[2], event->mac[3],event->mac[4], event->mac[5],
                 (int)event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station %02x:%02x:%02x:%02x:%02x:%02x leave, AID=%d",
                 event->mac[0], event->mac[1],event->mac[2], event->mac[3],event->mac[4], event->mac[5],
                 (int)event->aid);
    }
}

static void wifistation_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "retry to connect to the AP");
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));

        /*if (!m_bIsWebServerInit)
        {
            m_bIsWebServerInit = true;
            WEBSERVER_Init();
        }*/
    }
}

static void wifi_init_all(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    const bool isWiFiSTA = NVSJSON_GetValueInt32(&g_sSettingHandle, SETTINGS_EENTRY_WSTAIsActive) == 1;
    if (isWiFiSTA)
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );
    }
    else
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP) );
    }

    // Access point mode
    m_pWifiSoftAP = esp_netif_create_default_wifi_ap();

    esp_netif_ip_info_t ipInfo;
    IP4_ADDR(&ipInfo.ip, 192, 168, 66, 1);
	IP4_ADDR(&ipInfo.gw, 192, 168, 66, 1);
	IP4_ADDR(&ipInfo.netmask, 255, 255, 255, 0);
	esp_netif_dhcps_stop(m_pWifiSoftAP);
	esp_netif_set_ip_info(m_pWifiSoftAP, &ipInfo);
	esp_netif_dhcps_start(m_pWifiSoftAP);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_configAP = {
        .ap = {
            //.ssid = FWCONFIG_SOFTAP_WIFI_SSID,
            //.ssid_len = strlen(FWCONFIG_SOFTAP_WIFI_SSID),
            .channel = FWCONFIG_SOFTAP_WIFI_CHANNEL,
            .password = FWCONFIG_SOFTAP_WIFI_PASS,
            .max_connection = FWCONFIG_SOFTAP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    uint8_t macAddr[6];
    esp_read_mac(macAddr, ESP_MAC_WIFI_SOFTAP);

    sprintf((char*)wifi_configAP.ap.ssid, FWCONFIG_SOFTAP_WIFI_SSID_BASE, macAddr[3], macAddr[4], macAddr[5]);
    strcpy((char*)m_szSoftAPSSID, (char*)wifi_configAP.ap.ssid);

    int n = strlen((const char*)wifi_configAP.ap.ssid);
    wifi_configAP.ap.ssid_len = n;

    if (strlen((const char*)FWCONFIG_SOFTAP_WIFI_PASS) == 0) {
        wifi_configAP.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_configAP));

    // Soft Access Point Mode
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             wifi_configAP.ap.ssid, FWCONFIG_SOFTAP_WIFI_PASS, (int)FWCONFIG_SOFTAP_WIFI_CHANNEL);

    if (isWiFiSTA)
    {
        m_pWifiSTA = esp_netif_create_default_wifi_sta();

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifistation_event_handler,
                                                            NULL,
                                                            &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifistation_event_handler,
                                                            NULL,
                                                            &instance_got_ip));

        wifi_config_t wifi_configSTA = {
            .sta = {
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,

                .pmf_cfg = {
                    .capable = true,
                    .required = false
                },
            },
        };

        size_t staSSIDLength = 32;
        NVSJSON_GetValueString(&g_sSettingHandle, SETTINGS_EENTRY_WSTASSID, (char*)wifi_configSTA.sta.ssid, &staSSIDLength);

        size_t staPassLength = 64;
        NVSJSON_GetValueString(&g_sSettingHandle, SETTINGS_EENTRY_WSTAPass, (char*)wifi_configSTA.sta.password, &staPassLength);

        ESP_LOGI(TAG, "STA mode is active, attempt to connect to ssid: %s", wifi_configSTA.sta.ssid);

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_configSTA) );
    }

    // Start AP + STA
    ESP_ERROR_CHECK(esp_wifi_start() );
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    SETTINGS_Init();

    NVSJSON_Load(&g_sSettingHandle);

    // Need to be high ...
    GPIO_Init();

    SOUNDFX_Init();

    GATESTEPPER_Init();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "wifi_init_all");
    // wifi_init_softap();
    wifi_init_all();

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_init();

    GATECONTROL_Init();

    GATECONTROL_Start();

    strcpy(m_sSetting.dstRingAddress, FWCONFIG_RING_IPADDRESS);
    SGUBRCOMM_Init(&g_sSGUBRCOMMHandle, &m_sSetting);
    SGUBRCOMM_Start(&g_sSGUBRCOMMHandle);
    // GATECONTROL_DoAction(GATECONTROL_EMODE_GoHome);

    // const TickType_t xFrequency = 1;

     // Initialise the xLastWakeTime variable with the current time.
    // TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xLEDBlinkTicks = xTaskGetTickCount();
    TickType_t xPrintTimeTicks = 0;
    #ifdef HWCONFIG_OLED_ISPRESENT
    TickType_t xUpdateOLEDTicks = 0;
    #endif

    bool bAltern = false;

    WEBSERVER_Init();

    char* szAllTask = (char*)malloc(4096);
    vTaskList(szAllTask);
    ESP_LOGI(TAG, "vTaskList: \r\n\r\n%s", szAllTask);
    free(szAllTask);

    while(true)
    {
        #ifdef HWCONFIG_OLED_ISPRESENT
        // Update OLED screen
        if ( (xTaskGetTickCount() - xUpdateOLEDTicks) > pdMS_TO_TICKS(250) )
        {
            SSD1306_handle* pss1306Handle = GPIO_GetSSD1306Handle();
            const char szText[128+1];

            esp_netif_ip_info_t wifiIpSta = {0};
            MAIN_GetWiFiSTAIP(&wifiIpSta);

            esp_netif_ip_info_t wifiIpAP = {0};
            MAIN_GetWiFiSoftAPIP(&wifiIpAP);

            sprintf(szText, "%s\n"IPSTR"\n"IPSTR,
                m_szSoftAPSSID,
                IP2STR(&wifiIpAP.ip),
                IP2STR(&wifiIpSta.ip));

            SSD1306_ClearDisplay(pss1306Handle);
            SSD1306_DrawString(pss1306Handle, 0, 0, szText, strlen(szText));
            SSD1306_UpdateDisplay(pss1306Handle);
            xUpdateOLEDTicks = xTaskGetTickCount();
        }
        #endif

        // Wait for the next cycle.
        vTaskDelay(pdMS_TO_TICKS(1));
        //vTaskDelayUntil( &xLastWakeTime, xFrequency );

        if ( (xTaskGetTickCount() - xLEDBlinkTicks) > pdMS_TO_TICKS(250) )
        {
            GPIO_SetSanityLEDStatus(bAltern);
            bAltern = !bAltern;
            xLEDBlinkTicks = xTaskGetTickCount();
        }

        if ( (xTaskGetTickCount() - xPrintTimeTicks) > pdMS_TO_TICKS(20*1000) )
        {
            PrintCurrentTime();
            xPrintTimeTicks = xTaskGetTickCount();
        }

        if (m_xRebootRequestTicks != 0 && (xTaskGetTickCount() - m_xRebootRequestTicks) > pdMS_TO_TICKS(1500))
        {
            ESP_LOGI(TAG, "About to restart ...");
            esp_restart();
        }
    }
}

void MAIN_RequestReboot()
{
    m_xRebootRequestTicks = xTaskGetTickCount();
}


void MAIN_GetWiFiSTAIP(esp_netif_ip_info_t* pIPInfo)
{
    if (m_pWifiSTA != NULL)
        esp_netif_get_ip_info(m_pWifiSTA, pIPInfo);
}

void MAIN_GetWiFiSoftAPIP(esp_netif_ip_info_t* pIPInfo)
{
    if (m_pWifiSoftAP != NULL)
        esp_netif_get_ip_info(m_pWifiSoftAP, pIPInfo);
}

static void PrintCurrentTime()
{
    // Set timezone to Eastern Standard Time and print local time
    time_t now = 0;
    struct tm timeinfo = { 0 };
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "The current date/time in New York is: %2d:%2d:%2d", (int)timeinfo.tm_hour, (int)timeinfo.tm_min, (int)timeinfo.tm_sec);
}

static void time_sync_notification_cb(struct timeval* tv)
{
    // settimeofday(tv, NULL);
    ESP_LOGI(TAG, "Notification of a time synchronization event, sec: %d", (int)tv->tv_sec);
    PrintCurrentTime();
}