#include "GateStepper.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define TAG "GateStepper"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

static void tmr_signal_callback(void* arg);

#define STEPEND_BIT    0x01

// Related to period timer
static volatile bool m_bPeriodAlternate = false;
static volatile int32_t m_period = 0;

// Counter
static volatile int32_t m_count = 0;
static int32_t m_target = 0;

static esp_timer_handle_t m_sSignalTimerHandle;
static TaskHandle_t m_hCurrentTaskHandle;

void GATESTEPPER_Init()
{
    ESP_LOGI(TAG, "GATESTEPPER_Init");

    gpio_set_direction(FWCONFIG_STEPPER_DIR_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(FWCONFIG_STEPPER_STEP_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(FWCONFIG_STEPPER_SLP_PIN, GPIO_MODE_OUTPUT);

    // Activated
    gpio_set_level(FWCONFIG_STEPPER_SLP_PIN, true);
    gpio_set_level(FWCONFIG_STEPPER_STEP_PIN, false);
    gpio_set_level(FWCONFIG_STEPPER_DIR_PIN, false);

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

    m_count = 0;
    m_target = abs(s32Ticks);

    m_period = 1;

    const bool bIsCCW = s32Ticks > 0;
    gpio_set_level(FWCONFIG_STEPPER_DIR_PIN, bIsCCW);

    ESP_ERROR_CHECK(esp_timer_start_once(m_sSignalTimerHandle, m_period));

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

    gpio_set_level(FWCONFIG_STEPPER_STEP_PIN, m_bPeriodAlternate);
    if (m_bPeriodAlternate)
    {
        const int32_t s32 = MIN(abs(m_count) , abs(m_target - m_count));

        if (s32 < 100)
            m_period = 1500;
        else if (s32 < 200)
            m_period = 1250;
        else if (s32 < 300)
            m_period = 1000;
        else if (s32 < 400)
            m_period = 750;
        else if (s32 < 500)
            m_period = 675;
        else if (s32 < 600)
            m_period = 600;
        else
            m_period = 500;

        // Count every two
        m_count++;
        if (m_target == m_count)
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
        ESP_ERROR_CHECK(esp_timer_start_once(m_sSignalTimerHandle, m_period));

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
