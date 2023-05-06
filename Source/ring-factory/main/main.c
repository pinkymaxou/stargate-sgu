/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "fwconfig.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "webserver.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "gpio.h"
#include "lwip/apps/netbiosns.h"
#include "lwip/ip4_addr.h"

static const char *TAG = "Main";

static volatile int m_iConnectedCount = 0;
static bool m_bIsSuicide = false;

static esp_netif_t* m_pWifiSoftAP;
static wifi_config_t m_wifi_config;

static esp_pm_lock_handle_t m_lockHandle;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    m_pWifiSoftAP = esp_netif_create_default_wifi_ap();
    esp_netif_ip_info_t ipInfo;
    IP4_ADDR(&ipInfo.ip, 192, 168, 66, 1);
	IP4_ADDR(&ipInfo.gw, 192, 168, 66, 1);
	IP4_ADDR(&ipInfo.netmask, 255, 255, 255, 0);
	esp_netif_dhcps_stop(m_pWifiSoftAP);
	esp_netif_set_ip_info(m_pWifiSoftAP, &ipInfo);
	esp_netif_dhcps_start(m_pWifiSoftAP);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    uint8_t macAddr[6];
    esp_read_mac(macAddr, ESP_MAC_WIFI_SOFTAP);
    m_wifi_config.ap.channel = CONFIG_ESP_WIFI_CHANNEL;
    strcpy((char*)m_wifi_config.ap.password, CONFIG_ESP_WIFI_PASS);
    m_wifi_config.ap.max_connection = CONFIG_MAX_STA_CONN;
    m_wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;


    sprintf((char*)m_wifi_config.ap.ssid, CONFIG_ESP_WIFI_SSID_BASE, macAddr[3], macAddr[4], macAddr[5]);
    int n = strlen((const char*)m_wifi_config.ap.ssid);
    m_wifi_config.ap.ssid_len = n;

    if (strlen((const char*)CONFIG_ESP_WIFI_PASS) == 0) {
        m_wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_LOGI(TAG, "About to create soft access point ...");

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &m_wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());


    char ipString[12+1];
    esp_ip4addr_ntoa(&ipInfo.ip, ipString, sizeof(ipString));

    ESP_LOGI(TAG, "AP IP Address: %s", ipString);
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             m_wifi_config.ap.ssid, CONFIG_ESP_WIFI_PASS, (int)CONFIG_ESP_WIFI_CHANNEL);
}

static void LightningTask(void *pvParameters)
{
    bool b = false;

    // Complete shutdown by default
    GPIO_ClearAllPixels();
    ESP_ERROR_CHECK(esp_pm_lock_acquire(m_lockHandle));
    GPIO_RefreshPixels();
    ESP_ERROR_CHECK(esp_pm_lock_release(m_lockHandle));
    
    int currLedIndex = 0;
    bool isBlink = false;
    while(true)
    {
        // Change color by state
        if (isBlink)
        {
            if (m_bIsSuicide) // RED = means time to die
                GPIO_SetPixel(0, 255, 0, 0);
            else if (WEBSERVER_GetIsReceivingOTA()) // Yellow = Receiving
                GPIO_SetPixel(0, 255, 255, 0);
            else if (m_iConnectedCount > 0) // Green = If one client is connected
                GPIO_SetPixel(0, 0, 255, 0);
            else GPIO_SetPixel(0, 0, 0, 255); // Blue by default
        }
        else
            GPIO_SetPixel(0, 80, 80, 80);

        isBlink = !isBlink;

        // Make a LED Spin arround, except the first one
        for(int i = 1; i < CONFIG_WS1228B_LEDCOUNT; i++)
        {
            if (i == currLedIndex || i == ((CONFIG_WS1228B_LEDCOUNT) - currLedIndex))
            {
                if (i%5 == 0)
                    GPIO_SetPixel(i, 80, 80, 80);
                else
                    GPIO_SetPixel(i, 20, 20, 20);
            }
            else
                GPIO_SetPixel(i, 0, 0, 0);
        }
        currLedIndex = (currLedIndex + 1) % CONFIG_WS1228B_LEDCOUNT;

        ESP_ERROR_CHECK(esp_pm_lock_acquire(m_lockHandle));
        GPIO_RefreshPixels();
        ESP_ERROR_CHECK(esp_pm_lock_release(m_lockHandle));

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    GPIO_Init();

    ESP_ERROR_CHECK(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "NoLightSleep", &m_lockHandle));


    long ticks = xTaskGetTickCount();

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "wifi_init_softap");
    wifi_init_softap();

    WEBSERVER_Init();
    //const SIMPLEOTA_Params params = SIMPLEOTA_PARAM_DEFAULT;
    //SIMPLEOTA_StartAsync(&params);

    long switchTicks = 0;

    xTaskCreatePinnedToCore(&LightningTask, "blinkLeds", 4000, NULL, 5, NULL, 0);
    
    while(true)
    {
        if (!m_bIsSuicide)
        {
            const bool bSwitchState = gpio_get_level(CONFIG_SWITCH_PIN);
            if (!bSwitchState) // Up
            {
                if (switchTicks == 0)
                    switchTicks = xTaskGetTickCount();

                // If we hold the switch long enough it stop the process.
                if ( (xTaskGetTickCount() - switchTicks) > pdMS_TO_TICKS(CONFIG_SWITCH_HOLDDELAY_MS))
                {
                    m_bIsSuicide = true;
                    switchTicks = 0; // Reset;
                }
            }
            else
                switchTicks = 0; // Reset

            // Kill the power after 10 minutes maximum
            if (xTaskGetTickCount() - ticks > pdMS_TO_TICKS(CONFIG_HOLDPOWER_DELAY_MS))
            {
                m_bIsSuicide = true;
            }
        }

        // Means cutting to power to itself.
        if (m_bIsSuicide)
        {
            // Release the power pin
            gpio_set_level(CONFIG_HOLDPOWER_PIN, false);
        }

        // 4 HZ
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        m_iConnectedCount++;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), (int)event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        m_iConnectedCount--;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), (int)event->aid);
    }
}
