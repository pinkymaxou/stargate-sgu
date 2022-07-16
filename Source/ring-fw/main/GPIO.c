#include "GPIO.h"
#include "FWConfig.h"
#include "driver/gpio.h"

void GPIO_Init()
{
    // Hold the power pin on start-up
	gpio_pad_select_gpio((gpio_num_t)FWCONFIG_HOLDPOWER_PIN);
	gpio_set_direction((gpio_num_t)FWCONFIG_HOLDPOWER_PIN, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio((gpio_num_t)FWCONFIG_SWITCH_PIN);
	gpio_set_direction((gpio_num_t)FWCONFIG_SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)FWCONFIG_SWITCH_PIN, GPIO_PULLUP_ONLY);
    
}

void GPIO_EnableHoldPowerPin(bool bEnabled)
{
    gpio_set_level((gpio_num_t)FWCONFIG_HOLDPOWER_PIN, bEnabled);
}
