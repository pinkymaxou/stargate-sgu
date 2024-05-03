#include "driver/rmt.h"
#include "led_strip.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "gpio.h"
#include "fwconfig.h"

#define TAG "GPIO"

static led_strip_handle_t led_strip;

void GPIO_Init()
{
    // Hold the power pin on start-up
	//gpio_pad_select_gpio(CONFIG_HOLDPOWER_PIN);
	gpio_set_direction(CONFIG_HOLDPOWER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_HOLDPOWER_PIN, true);

    // Configure the switch
	//gpio_pad_select_gpio(CONFIG_SWITCH_PIN);
	gpio_set_direction(CONFIG_SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(CONFIG_SWITCH_PIN, GPIO_PULLUP_ONLY);

    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_WS1228B_PIN,
        .max_leds = CONFIG_WS1228B_LEDCOUNT, // sanity LED + at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
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