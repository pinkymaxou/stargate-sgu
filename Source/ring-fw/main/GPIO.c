#include "gpio.h"
#include "FWConfig.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define TAG "GPIO"

static led_strip_t* m_strip = NULL;

void GPIO_Init()
{
    // Hold the power pin on start-up
	gpio_set_direction((gpio_num_t)FWCONFIG_HOLDPOWER_PIN, GPIO_MODE_OUTPUT);

	gpio_set_direction((gpio_num_t)FWCONFIG_SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)FWCONFIG_SWITCH_PIN, GPIO_PULLUP_ONLY);

    // Initialize LED drivers
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(FWCONFIG_WS1228B_PIN, FWCONFIG_WS1228B_RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // -----------------------
    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(HWCONFIG_WS1228B_LEDCOUNT, (led_strip_dev_t)config.channel);
    m_strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!m_strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }
    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(m_strip->clear(m_strip, 100));
}

void GPIO_EnableHoldPowerPin(bool bEnabled)
{
    gpio_set_level((gpio_num_t)FWCONFIG_HOLDPOWER_PIN, bEnabled);
}

void GPIO_SetPixel(uint32_t u32Index, uint8_t u8Red, uint8_t u8Green, uint8_t u8Blue)
{
    ESP_ERROR_CHECK(m_strip->set_pixel(m_strip, u32Index, u8Red, u8Green, u8Blue));
}

void GPIO_ClearAllPixels()
{
    ESP_ERROR_CHECK(m_strip->clear(m_strip, 50));
}

void GPIO_RefreshPixels()
{
    ESP_ERROR_CHECK(m_strip->refresh(m_strip, 100));
}