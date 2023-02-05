#include <stdio.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define TAG "test-stepper"

#define FWCONFIG_STEPPER_DIR_PIN GPIO_NUM_33
#define FWCONFIG_STEPPER_STEP_PIN GPIO_NUM_25
#define FWCONFIG_STEPPER_SLP_PIN GPIO_NUM_26

static void periodic_timer_callback(void* arg);

static bool m_b = false;
static int32_t m_period = 0;

static int32_t m_count = 0;
static int32_t m_target = 0;

static esp_timer_handle_t m_periodic_timer;

void app_main(void)
{
    gpio_set_direction(FWCONFIG_STEPPER_DIR_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(FWCONFIG_STEPPER_STEP_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(FWCONFIG_STEPPER_SLP_PIN, GPIO_MODE_OUTPUT);

    // Activated
    gpio_set_level(FWCONFIG_STEPPER_SLP_PIN, true);
    gpio_set_level(FWCONFIG_STEPPER_STEP_PIN, false);
    gpio_set_level(FWCONFIG_STEPPER_DIR_PIN, false);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback,
        .name = "periodic"
    };

    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &m_periodic_timer));

    /*  Speed 
        Accel : 400 t/s
        Top speed: 1000 t/s
    */

    /* Start the timers */
    m_period = 500;
    m_count = 0;
    m_target = 7000;
    gpio_set_level(FWCONFIG_STEPPER_STEP_PIN, m_b);
    ESP_ERROR_CHECK(esp_timer_start_periodic(m_periodic_timer, m_period));
}

static int32_t GetPeriod(int32_t s32Count, int32_t s32Target)
{
    const int32_t diff = abs(s32Target - s32Count);
    if (s32Count < 500)
        return 500;
    if (s32Count < 10000)
        return 500;
    return 0;
}

//     int64_t time_since_boot = esp_timer_get_time();
static IRAM_ATTR void periodic_timer_callback(void* arg)
{
    gpio_set_level(FWCONFIG_STEPPER_STEP_PIN, m_b);
    if (m_b)
    {
        m_count++;
        if (m_target == m_count)
        {
            esp_timer_stop(m_periodic_timer);
        }
    }
    m_b = !m_b;
}
