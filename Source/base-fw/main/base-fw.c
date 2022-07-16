/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <esp_event.h>
#include <esp_log.h>
#include "lwip/err.h"
#include "lwip/sys.h"
#include "webserver.h"
#include "GPIO.h"
#include "FWConfig.h"
#include "Settings.h"
#include "base-fw.h"

#include "GateControl.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

static const char *TAG = "wifi";

SGUBRCOMM_SHandle g_sSGUBRCOMMHandle;
static SGUBRCOMM_SSetting m_sSetting = { .eMode = SGUBRCOMM_EMODE_Base };
static TickType_t m_xRebootRequestTicks = 0;

static void wifi_init_all(void);
static void wifi_init_softap(void);

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifistation_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
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
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );

    // Access point mode
    esp_netif_t* wifiAP = esp_netif_create_default_wifi_ap();

    esp_netif_ip_info_t ipInfo;
    IP4_ADDR(&ipInfo.ip, 192, 168, 66,1);
	IP4_ADDR(&ipInfo.gw, 192, 168, 66,1);
	IP4_ADDR(&ipInfo.netmask, 255, 255, 255, 0);
	esp_netif_dhcps_stop(wifiAP);
	esp_netif_set_ip_info(wifiAP, &ipInfo);
	esp_netif_dhcps_start(wifiAP);

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
    int n = strlen((const char*)wifi_configAP.ap.ssid);
    wifi_configAP.ap.ssid_len = n;

    if (strlen((const char*)FWCONFIG_SOFTAP_WIFI_PASS) == 0) {
        wifi_configAP.ap.authmode = WIFI_AUTH_OPEN;
    }

    //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_configAP));


    // Soft Access Point Mode
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             wifi_configAP.ap.ssid, FWCONFIG_SOFTAP_WIFI_PASS, FWCONFIG_SOFTAP_WIFI_CHANNEL);

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
            .ssid = FWCONFIG_STA_WIFI_SSID,
            .password = FWCONFIG_STA_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_configSTA) );

    //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );

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
    //SETTINGS_Load();

    // Need to be high ...
    GPIO_Init();

    GPIO_StartStepper();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "wifi_init_all");
    // wifi_init_softap();
    wifi_init_all();

    GATECONTROL_Init();

    GATECONTROL_Start();

    strcpy(m_sSetting.dstRingAddress, FWCONFIG_RING_IPADDRESS);
    SGUBRCOMM_Init(&g_sSGUBRCOMMHandle, &m_sSetting);
    SGUBRCOMM_Start(&g_sSGUBRCOMMHandle);
    // GATECONTROL_DoAction(GATECONTROL_EMODE_GoHome);

    const TickType_t xFrequency = 1;

     // Initialise the xLastWakeTime variable with the current time.
    TickType_t xLastWakeTime = xTaskGetTickCount();

    TickType_t xLEDBlinkTicks = xTaskGetTickCount();
    bool bAltern = false;

    WEBSERVER_Init();

    while(true)
    {
        // Wait for the next cycle.
        vTaskDelay(pdMS_TO_TICKS(1));
        //vTaskDelayUntil( &xLastWakeTime, xFrequency );

        if ( (xTaskGetTickCount() - xLEDBlinkTicks) > pdMS_TO_TICKS(250) )
        {
            GPIO_SetSanityLEDStatus(bAltern);
            bAltern = !bAltern;
            xLEDBlinkTicks = xTaskGetTickCount();
        }

        if (m_xRebootRequestTicks != 0 && (xTaskGetTickCount() - m_xRebootRequestTicks) > pdMS_TO_TICKS(1500))
        {
            ESP_LOGI(TAG, "About to restart ...");
            esp_restart();
        }
    }
}

void BASEFW_RequestReboot()
{
    m_xRebootRequestTicks = xTaskGetTickCount();
}