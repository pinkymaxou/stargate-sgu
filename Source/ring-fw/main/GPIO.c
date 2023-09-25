#include "gpio.h"
#include "FWConfig.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define TAG "GPIO"

static led_strip_handle_t led_strip;

void GPIO_Init()
{
    // Hold the power pin on start-up
	gpio_set_direction((gpio_num_t)FWCONFIG_HOLDPOWER_PIN, GPIO_MODE_OUTPUT);

	gpio_set_direction((gpio_num_t)FWCONFIG_SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)FWCONFIG_SWITCH_PIN, GPIO_PULLUP_ONLY);


    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = FWCONFIG_WS1228B_PIN,
        .max_leds = HWCONFIG_WS1228B_LEDCOUNT, // sanity LED + at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

void GPIO_EnableHoldPowerPin(bool bEnabled)
{
    gpio_set_level((gpio_num_t)FWCONFIG_HOLDPOWER_PIN, bEnabled);
}

void GPIO_SetPixel(uint32_t u32Index, uint8_t u8Red, uint8_t u8Green, uint8_t u8Blue)
{
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, u32Index, u8Red, u8Green, u8Blue));
}

void GPIO_ClearAllPixels()
{
    ESP_ERROR_CHECK(led_strip_clear(led_strip));
}

void GPIO_RefreshPixels()
{
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}