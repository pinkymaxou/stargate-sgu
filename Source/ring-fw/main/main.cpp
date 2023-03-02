/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "FWConfig.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_pm.h"
#include "sdkconfig.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "FastLED.h"
#include "GPIO.h"
#include "SGUBRProtocol.h"
#include "SGUBRComm.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_ota_ops.h"

#define SGU_GATTS_TAG "SGU-GATTS"

#define LED_OUTPUT_MAX (220)
#define LED_OUTPUT_IDLE (100)

static void InitWS1228B();
static void InitESPNOW();

static CRGB m_leds[FWCONFIG_WS1228B_LEDCOUNT] = { CRGB::Black };

static const char *TAG = "Main";

const uint8_t m_u8BroadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static volatile bool m_bIsSuicide = false;
static volatile TickType_t m_lAutoOffTicks = xTaskGetTickCount();
static volatile TickType_t m_ulAutoOffTimeoutMs = FWCONFIG_HOLDPOWER_DELAY_MS;

// Chevron animation
static volatile int32_t m_s32ChevronAnim = -1;

static esp_pm_lock_handle_t m_lockHandle;

// These should survive to reset
static const uint8_t u8MagicNumber_OTAMode[4] = { 0xA0, 0xBA, 0xDC, 0xC0  };

static bool m_bIsOTAMode = false;

static RTC_IRAM_ATTR uint8_t m_u8StartModes[4];

extern "C" {
    void app_main();
    static void ResetAutoOffTicks();

    static void SGUBRKeepAliveHandler(const SGUBRPROTOCOL_SKeepAliveArg* psKeepAlive);
    static void SGUBRTurnOffHandler();
    static void SGUBRUpdateLightHandler(const SGUBRPROTOCOL_SUpdateLightArg* psArg);
    static void SGUBRChevronsLightningHandler(const SGUBRPROTOCOL_SChevronsLightningArg* psChevronLightningArg);
    static void SGUBRGotoFactory();
    static void SGUBRGotoOTAMode();
}

static const SGUBRPROTOCOL_SConfig m_sConfig =
{
    .fnKeepAliveHandler = SGUBRKeepAliveHandler,
    .fnTurnOffHandler = SGUBRTurnOffHandler,
    .fnUpdateLightHandler = SGUBRUpdateLightHandler,
    .fnChevronsLightningHandler = SGUBRChevronsLightningHandler,
    .fnGotoFactoryHandler = SGUBRGotoFactory,
    .fnGotoOTAModeHandler = SGUBRGotoOTAMode
};

static SGUBRCOMM_SHandle m_sSGUBRCommHandle;
static SGUBRCOMM_SSetting m_sSetting = { .eMode = SGUBRCOMM_EMODE_Ring, .psSGUBRProtocolConfig = &m_sConfig };

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifistation_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static void InitWS1228B()
{
    FastLED.addLeds<WS2812B, FWCONFIG_WS1228B_PIN, GRB>(m_leds, FWCONFIG_WS1228B_LEDCOUNT);
}

static void InitESPNOW()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    if (m_bIsOTAMode)
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );

        // Access point mode
        esp_netif_t* wifiAP = esp_netif_create_default_wifi_ap();

        esp_netif_ip_info_t ipInfo;
        IP4_ADDR(&ipInfo.ip, 192, 168, 55, 1);
        IP4_ADDR(&ipInfo.gw, 192, 168, 55, 1);
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
                .channel = FWCONFIG_SOFTAP_WIFI_CHANNEL,
                .max_connection = FWCONFIG_SOFTAP_MAX_CONN,
            },
        };
        wifi_configAP.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

        uint8_t macAddr[6];
        esp_read_mac(macAddr, ESP_MAC_WIFI_SOFTAP);
        sprintf((char*)wifi_configAP.ap.ssid, FWCONFIG_SOFTAP_WIFI_SSID_BASE, macAddr[3], macAddr[4], macAddr[5]);
        int n = strlen((const char*)wifi_configAP.ap.ssid);
        wifi_configAP.ap.ssid_len = n;

        if (strlen((const char*)FWCONFIG_SOFTAP_WIFI_PASS) == 0) {
            wifi_configAP.ap.authmode = WIFI_AUTH_OPEN;
        }
        else
        {
            strcpy((char*)wifi_configAP.ap.password, FWCONFIG_SOFTAP_WIFI_PASS);
        }

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_configAP));

        ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
                wifi_configAP.ap.ssid, (int)FWCONFIG_SOFTAP_WIFI_PASS, (int)FWCONFIG_SOFTAP_WIFI_CHANNEL);
    }
    else
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    }

    // Soft Access Point Mode
    esp_netif_t* wifiSTA = esp_netif_create_default_wifi_sta();

    esp_netif_ip_info_t ipInfoSTA;
    IP4_ADDR(&ipInfoSTA.ip, 192, 168, 66, 250);
	IP4_ADDR(&ipInfoSTA.gw, 192, 168, 66, 250);
	IP4_ADDR(&ipInfoSTA.netmask, 255, 255, 255, 0);
    esp_netif_dhcpc_stop(wifiSTA);
	esp_netif_set_ip_info(wifiSTA, &ipInfoSTA);

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
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    strcpy((char*)wifi_configSTA.sta.ssid, FWCONFIG_MASTERBASE_SSID);

    if (strlen(FWCONFIG_MASTERBASE_PASS) == 0)
    {
        wifi_configSTA.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    else
    {
        wifi_configSTA.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        strcpy((char*)wifi_configSTA.sta.password, FWCONFIG_MASTERBASE_PASS);
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_configSTA) );

    // Start AP + STA
    ESP_ERROR_CHECK(esp_wifi_start() );
}

static void LedRefreshTask(void *pvParameters)
{
    // First refresh
     // Initialize chevrons dimmed lit
    for(int i = 0; i < FWCONFIG_WS1228B_LEDCOUNT; i++)
    {
        if ((i % 5) == 0)
            m_leds[i] = CRGB(LED_OUTPUT_IDLE, LED_OUTPUT_IDLE, LED_OUTPUT_IDLE);
        else
            m_leds[i] = CRGB::Black;
    }

    while(true)
    {
        // Detect the switch status
        ESP_ERROR_CHECK(esp_pm_lock_acquire(m_lockHandle));
        FastLED.show();

        // Play chevron animation
        if (m_s32ChevronAnim >= 0)
        {
            switch((SGUBRPROTOCOL_ECHEVRONANIM)m_s32ChevronAnim)
            {
                case SGUBRPROTOCOL_ECHEVRONANIM_FadeIn:
                {
                    ESP_LOGI(TAG, "Animation / FadeIn");
                    for(int brightness = 0; brightness < LED_OUTPUT_MAX; brightness += 10)
                    {
                        for(int i = 0; i < FWCONFIG_WS1228B_LEDCOUNT; i++)
                        {
                            if ((i % 5) == 0)
                                m_leds[i] = CRGB(brightness, brightness, brightness);
                            else
                                m_leds[i] = CRGB::Black;
                        }
                        FastLED.show();
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                    break;
                }
                case SGUBRPROTOCOL_ECHEVRONANIM_FadeOut:
                {
                    ESP_LOGI(TAG, "Animation / FadeOut");
                    for(int brightness = LED_OUTPUT_MAX; brightness >= 0; brightness -= 10)
                    {
                        for(int i = 0; i < FWCONFIG_WS1228B_LEDCOUNT; i++)
                        {
                            if ((i % 5) == 0)
                                m_leds[i] = CRGB(brightness, brightness, brightness);
                            else
                                m_leds[i] = CRGB::Black;
                        }
                        FastLED.show();
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                    break;
                }
                case SGUBRPROTOCOL_ECHEVRONANIM_ErrorToWhite:
                {
                    ESP_LOGI(TAG, "Animation / Error");
                    for(int i = 0; i < FWCONFIG_WS1228B_LEDCOUNT; i++)
                    {
                        if ((i % 5) == 0)
                            m_leds[i] = CRGB(LED_OUTPUT_MAX, 0, 0);
                        else
                            m_leds[i] = CRGB::Black;
                    }

                    FastLED.show();
                    vTaskDelay(pdMS_TO_TICKS(1500));

                    for(int brightness = 0; brightness < LED_OUTPUT_MAX; brightness += 10)
                    {
                        for(int i = 0; i < FWCONFIG_WS1228B_LEDCOUNT; i++)
                        {
                            if ((i % 5) == 0)
                                m_leds[i] = CRGB(LED_OUTPUT_MAX, brightness, brightness);
                        }
                        FastLED.show();
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                    break;
                }
                case SGUBRPROTOCOL_ECHEVRONANIM_ErrorToOff:
                {
                    ESP_LOGI(TAG, "Animation / Error");
                    for(int i = 0; i < FWCONFIG_WS1228B_LEDCOUNT; i++)
                    {
                        if ((i % 5) == 0)
                            m_leds[i] = CRGB(LED_OUTPUT_MAX, 0, 0);
                        else
                            m_leds[i] = CRGB::Black;
                    }

                    FastLED.show();
                    vTaskDelay(pdMS_TO_TICKS(1500));

                    for(int brightness = LED_OUTPUT_MAX; brightness >= 0; brightness -= 10)
                    {
                        for(int i = 0; i < FWCONFIG_WS1228B_LEDCOUNT; i++)
                        {
                            if ((i % 5) == 0)
                                m_leds[i] = CRGB(brightness, 0, 0);
                        }
                        FastLED.show();
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                    break;
                }
                default:
                    ESP_LOGE(TAG, "Unknown animation");
                    break;
            }

            m_s32ChevronAnim = -1; // No more animation to process
        }

        FastLED.show();
        ESP_ERROR_CHECK(esp_pm_lock_release(m_lockHandle));
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), (int)event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), (int)event->aid);
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
    }
}

static void ResetAutoOffTicks()
{
    m_lAutoOffTicks = xTaskGetTickCount();
    m_bIsSuicide = false;
}

static void SGUBRKeepAliveHandler(const SGUBRPROTOCOL_SKeepAliveArg* psKeepAliveArg)
{
    ESP_LOGI(TAG, "BLE Keep Alive received, resetting timer. Time out set at: %u", /*0*/(uint)psKeepAliveArg->u32MaximumTimeMS);
    m_ulAutoOffTimeoutMs = psKeepAliveArg->u32MaximumTimeMS + (psKeepAliveArg->u32MaximumTimeMS/2);
    ResetAutoOffTicks();
}

static void SGUBRTurnOffHandler()
{
    ESP_LOGI(TAG, "BLE Turn Off received");
    m_bIsSuicide = true;
}

static void SGUBRUpdateLightHandler(const SGUBRPROTOCOL_SUpdateLightArg* psArg)
{
    ESP_LOGI(TAG, "BLE Update light received. Lights: %u", /*0*/(uint)psArg->u8LightCount);

     // Keep chevrons dimly lit
    for(int i = 0; i < psArg->u8LightCount; i++)
    {
        uint8_t u8LightIndex = psArg->u8Lights[i];

        if (u8LightIndex >= FWCONFIG_WS1228B_LEDCOUNT)
        {
            ESP_LOGE(TAG, "Invalid light index, %u", /*0*/u8LightIndex);
            continue;
        }

        m_leds[u8LightIndex] = CRGB(psArg->sRGB.u8Red, psArg->sRGB.u8Green, psArg->sRGB.u8Blue);
        // ESP_LOGI(TAG, "Led index change: %d, Red: %d, Green: %d, Blue: %d", u8LightIndex, psArg->sRGB.u8Red, psArg->sRGB.u8Green, psArg->sRGB.u8Blue);
    }

    ResetAutoOffTicks();
}

static void SGUBRChevronsLightningHandler(const SGUBRPROTOCOL_SChevronsLightningArg* psChevronLightningArg)
{
    ESP_LOGI(TAG, "BLE Chevron light received");

    // Not ready for chevron animation yet.
    m_s32ChevronAnim = psChevronLightningArg->eChevronAnim;
    ResetAutoOffTicks();
}

static void SGUBRGotoFactory()
{
    ESP_LOGI(TAG, "Goto factory mode");

    const esp_partition_t* pFactoryPartition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);

    if (pFactoryPartition == NULL)
    {
        ESP_LOGE(TAG, "Factory partition cannot be found");
        return;
    }

    ESP_LOGI(TAG, "Set boot partition to factory mode");
    esp_ota_set_boot_partition(pFactoryPartition);
    esp_restart();

    ResetAutoOffTicks();
}

static void SGUBRGotoOTAMode()
{
    ESP_LOGI(TAG, "Goto OTA mode");

    memcpy(m_u8StartModes, u8MagicNumber_OTAMode, 4);

    esp_restart();
    ResetAutoOffTicks();
}

static void MainTask(void *pvParameters)
{
    ESP_LOGI(TAG, "MainTask started ...");

    while(true)
    {
        // The function is blocking so it's fine
        SGUBRCOMM_Process(&m_sSGUBRCommHandle);

        // 100 HZ
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "NoLightSleep", &m_lockHandle));

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Check if the magic number is right, then clear the flag.
    m_bIsOTAMode = memcmp(m_u8StartModes, u8MagicNumber_OTAMode, 4) == 0;
    memset(m_u8StartModes, 0, 4);
    if (m_bIsOTAMode)
    {
        ESP_LOGI(TAG, "OTA mode is activated");
    }

    GPIO_Init();
    GPIO_EnableHoldPowerPin(true);

    InitWS1228B();

    InitESPNOW();

    SGUBRCOMM_Init(&m_sSGUBRCommHandle, &m_sSetting);
    SGUBRCOMM_Start(&m_sSGUBRCommHandle);
    // Clear LED strip (turn off all LEDs)
    //ESP_ERROR_CHECK(esp_pm_lock_acquire(lockHandle));
    //ESP_ERROR_CHECK(esp_pm_lock_release(lockHandle));

    xTaskCreatePinnedToCore(&LedRefreshTask, "RefreshLEDs", 4000, NULL, 10, NULL, 0);

    // Create task on CPU one ... to not interfere with FastLED
    // AppMain is created on task 0 by default.
    xTaskCreatePinnedToCore(&MainTask, "MainTask", 4000, NULL, 10, NULL, 1);

    long switchTicks = 0;

    while(true)
    {
        if (!m_bIsSuicide)
        {
            const bool bSwitchState = gpio_get_level((gpio_num_t)FWCONFIG_SWITCH_PIN);
            if (!bSwitchState) // Up
            {
                if (switchTicks == 0)
                    switchTicks = xTaskGetTickCount();

                // If we hold the switch long enough it stop the process.
                if ( (xTaskGetTickCount() - switchTicks) > pdMS_TO_TICKS(FWCONFIG_SWITCH_HOLDDELAY_MS))
                {
                    m_bIsSuicide = true;
                    switchTicks = 0; // Reset;
                }
            }
            else
                switchTicks = 0; // Reset

            // Kill the power after 10 minutes maximum
            if ((xTaskGetTickCount() - m_lAutoOffTicks) > pdMS_TO_TICKS(m_ulAutoOffTimeoutMs))
            {
                m_bIsSuicide = true;
            }
        }

        // Means cutting to power to itself.
        if (m_bIsSuicide)
        {
            // Release the power pin
            // m_s32ChevronAnim = SGUBRPROTOCOL_ECHEVRONANIM_ErrorToOff;
            // Delay for animation before stop
            vTaskDelay(pdMS_TO_TICKS(2500));
            GPIO_EnableHoldPowerPin(false);

            ESP_LOGW(TAG, "Time to die, let me die in peace");
            m_bIsSuicide = false;
        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
