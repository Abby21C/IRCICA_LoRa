#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "sx126x.h"
#include "sx1261_WuR.h"
#include "payload_manager.h"
#include "led_status.h"

#include "wur_transmission.h"

#include "config.h"

static const char *TAG = "wur_transmission";

// SX126x context passed from external module (used for SPI / GPIO communication)
static context_t *sx_parameters = NULL;

// 8-byte Sync Word
//static uint8_t SYNCWORD[] = {0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae};
//const uint8_t syncword[SYNCWORD_LEN] = SYNCWORD;
static const uint8_t syncword[SYNCWORD_LEN] = SYNCWORD;


//#define PAYLOAD_TYPE_OTA_VERSION 0x04



/**
 * @brief Initialize SX1261 and enter Wake-up Receiver (WuR) standby mode
 */
static void wur_init(void)
{
    if (sx_parameters == NULL) {
        ESP_LOGE(TAG, "wur_init: sx_parameters is NULL!");
        return;
    }

    sx126x_reset(sx_parameters);
    ESP_LOGI(TAG, "Initializing SX1261 parameters for WuR...");
    Sx1261_InitParam_WUR(sx_parameters, syncword);
    Sx1261_InitFonction_WUR(sx_parameters);
}

/**
 * @brief TX task function: send wake-up signal every 10 seconds
 */
static void run_tx_task(void *pvParameters)
{
    uint8_t counter = 0;

    while (1)
    {
        send_payload(PAYLOAD_TYPE_OTA_VERSION); 
        ESP_LOGI(TAG, "TX: Payload sent, count = %d", ++counter);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/**
 * @brief Start TX mode, pass in the initialized context
 */
void wur_start_tx(context_t *external_context)
{
    if (external_context == NULL) {
        ESP_LOGE(TAG, "wur_start_tx: external_context is NULL!");
        return;
    }

    sx_parameters = external_context;
    wur_init();
    xTaskCreate(run_tx_task, "run_tx_task", 4096, NULL, 5, NULL);
}

/**
 * @brief RX task function (used for continuous listening mode)
 */
static void run_rx_task(void *pvParameters)
{
    sx126x_irq_mask_t irq_mask;
    uint64_t start_time = esp_timer_get_time();
    int wake_count = 0;

    while (1)
    {
        sx126x_get_irq_status(sx_parameters, &irq_mask);

        if (irq_mask & SX126X_IRQ_SYNC_WORD_VALID)
        {
            wake_count++;
            uint64_t now = esp_timer_get_time();
            ESP_LOGI(TAG, "RX: Wake #%d at %llu sec", wake_count, (now - start_time) / 1000000);

            led_status_toggle();
            vTaskDelay(pdMS_TO_TICKS(1000));
            led_status_toggle();

            receive_payload();

            sx126x_clear_irq_status(sx_parameters, &irq_mask);
            sx126x_reset(sx_parameters);
            Sx1261_InitParam_WUR(sx_parameters, syncword);
            Sx1261_InitFonction_WUR(sx_parameters);
            Sx1261_WurMode(sx_parameters, 999, 1);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Start RX continuous listening mode (for debugging)
 */
void wur_start_rx(context_t *external_context)
{
    if (external_context == NULL) {
        ESP_LOGE(TAG, "wur_start_rx: external_context is NULL!");
        return;
    }

    sx_parameters = external_context;
    wur_init();
    xTaskCreate(run_rx_task, "run_rx_task", 4096, NULL, 5, NULL);
}

/**
 * @brief Perform a one-time RX reception after wake-up and reconfigure WuR mode (used in low-power mode)
 */
void wur_rx_once(context_t *external_context)
{
    if (external_context == NULL) {
        ESP_LOGE(TAG, "wur_rx_once: external_context is NULL!");
        return;
    }

    sx_parameters = external_context;

    ESP_LOGI(TAG, "wur_rx_once: Wake-up triggered. Receiving payload...");
    receive_payload();


    ESP_LOGI(TAG, "wur_rx_once: Reinitializing SX1261 for WuR mode");
    sx126x_reset(sx_parameters);
    Sx1261_InitParam_WUR(sx_parameters, syncword);
    Sx1261_InitFonction_WUR(sx_parameters);
    Sx1261_WurMode(sx_parameters, 999, 1);
}
