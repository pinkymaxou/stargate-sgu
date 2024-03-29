#include "GateStepper.h"
#include "HelperMacro.h"
#include "HWConfig.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define TAG "GateStepper"

static void tmr_signal_callback(void* arg);

#define STEPEND_BIT    0x01

// Related to period timer
static volatile bool m_bPeriodAlternate = false;
static volatile int32_t m_s32Period = 0;

// Counter
static volatile int32_t m_s32Count = 0;
static int32_t m_s32Target = 0;

static esp_timer_handle_t m_sSignalTimerHandle;
static TaskHandle_t m_hCurrentTaskHandle;

void GATESTEPPER_Init()
{
    ESP_LOGI(TAG, "GATESTEPPER_Init");

    // Activated
    gpio_set_level(HWCONFIG_STEPPER_STEP_PIN, false);
    gpio_set_level(HWCONFIG_STEPPER_DIR_PIN, false);

    // m_sWaitReachHandle = xSemaphoreCreateBinary();

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &tmr_signal_callback,
        .name = "periodic",
        .dispatch_method = ESP_TIMER_ISR
    };

    /* Start the timers */
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &m_sSignalTimerHandle));
}

bool GATESTEPPER_MoveTo(int32_t s32Ticks)
{
    m_hCurrentTaskHandle = xTaskGetCurrentTaskHandle();

    m_bPeriodAlternate = false;

    m_s32Count = 0;
    m_s32Target = abs(s32Ticks);

    m_s32Period = 1;

    const bool bIsCCW = s32Ticks > 0;
    gpio_set_level(HWCONFIG_STEPPER_DIR_PIN, bIsCCW);

    ESP_ERROR_CHECK(esp_timer_start_once(m_sSignalTimerHandle, m_s32Period));

    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 30000 );

    /* Wait to be notified of an interrupt. */
    uint32_t ulNotifiedValue = 0;
    const BaseType_t xResult = xTaskNotifyWait(pdFALSE,    /* Don't clear bits on entry. */
                        ULONG_MAX,        /* Clear all bits on exit. */
                        &ulNotifiedValue, /* Stores the notified value. */
                        xMaxBlockTime );

    if( xResult != pdPASS )
    {
        ESP_LOGE(TAG, "wait done ...");
        return false;
    }

    return true;
}

//     int64_t time_since_boot = esp_timer_get_time();
static IRAM_ATTR void tmr_signal_callback(void* arg)
{
    static BaseType_t xHigherPriorityTaskWoken;
    bool bIsDone = false;

    xHigherPriorityTaskWoken = pdFALSE;

    gpio_set_level(HWCONFIG_STEPPER_STEP_PIN, m_bPeriodAlternate);
    if (m_bPeriodAlternate)
    {
        const int32_t s32 = HELPERMACRO_MIN(abs(m_s32Count) , abs(m_s32Target - m_s32Count));
        /* Former code
        if (s32 < 50*2)
            m_s32Period = 2000/2;
        else if (s32 < 100*2)
            m_s32Period = 1800/2;
        else if (s32 < 200*2)
            m_s32Period = 1600/2;
        else if (s32 < 300*2)
            m_s32Period = 1400/2;
        else if (s32 < 400*2)
            m_s32Period = 1200/2;
        else if (s32 < 500*2)
            m_s32Period = 1000/2;
        else if (s32 < 600*2)
            m_s32Period = 800/2;
        else if (s32 < 700*2)
            m_s32Period = 600/2;
        else
            m_s32Period = 400/2;*/
        /* I just did some tests until I was satisfied */
        /* #1 (100, 1000), (1400, 300)
           a = -0.53846153846153846153846153846154
           b = 1053.84 */
        /* #2 (100, 700), (1400, 200)
           a = -0.3846
           b = 738.44 */
        const int32_t a = -461;
        const int32_t b = 946840;
        m_s32Period = (a * s32 + b)/1000;

        // I hoped it would reduce jitter.
        m_s32Period = (m_s32Period / 50) * 50;

        if (m_s32Period < 300)
            m_s32Period = 300;
        if (m_s32Period > 700)
            m_s32Period = 700;

        // Count every two
        m_s32Count++;
    }
    else
    {
        // Wait until the period go to low before considering it finished
        if (m_s32Target == m_s32Count)
        {
            xTaskNotifyFromISR( m_hCurrentTaskHandle,
                STEPEND_BIT,
                eSetBits,
                &xHigherPriorityTaskWoken );
            bIsDone = true;
        }
    }

    m_bPeriodAlternate = !m_bPeriodAlternate;
    if (!bIsDone)
        ESP_ERROR_CHECK(esp_timer_start_once(m_sSignalTimerHandle, m_s32Period));

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
