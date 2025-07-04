#include "utils.h"

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "sx126x.h"
#include "sx1261_WuR.h"
#include "esp_sleep.h"

// Pointer defined in the main function
extern context_t *sx_parameters;

// Global 8-byte sync word array
extern uint8_t SYNCWORD[];

#define TAG "utils"
#define CONFIG_DIO1_GPIO 6
#define CONFIG_NSS_GPIO 7

void print_sta_ip_address(void)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    if (netif == NULL) {
        ESP_LOGE("IP_CHECK", "Failed to get netif handle for STA.");
        return;
    }

    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        ESP_LOGI("IP_CHECK", "ESP32 IP Address: " IPSTR, IP2STR(&ip_info.ip));
        ESP_LOGI("IP_CHECK", "Netmask: " IPSTR, IP2STR(&ip_info.netmask));
        ESP_LOGI("IP_CHECK", "Gateway: " IPSTR, IP2STR(&ip_info.gw));
    } else {
        ESP_LOGE("IP_CHECK", "Failed to get IP info");
    }
}

void task_interruption(void *parameter)
{
    sx126x_irq_mask_t sx126x_irq_mask;
    ESP_LOGW(TAG, "DIO1 Level %d", gpio_get_level(CONFIG_DIO1_GPIO));

    while (true)
    {
        sx126x_get_irq_status(sx_parameters, &sx126x_irq_mask);
        if (sx126x_irq_mask & SX126X_IRQ_SYNC_WORD_VALID)
        {
            ESP_LOGW(TAG, "irq : %d", sx126x_irq_mask);
            ESP_LOGW(TAG, "DIO1 Level %d", gpio_get_level(CONFIG_DIO1_GPIO));
            ESP_LOGE(TAG, "WOKE UP IN TASK");

            vTaskDelay(pdMS_TO_TICKS(1000));

            ESP_LOGI(TAG, "Sx1261_WurMode");
            sx126x_clear_irq_status(sx_parameters, &sx126x_irq_mask);
            sx126x_reset(sx_parameters);
            ESP_LOGI(TAG, "Sx1261_InitParamRX");
            Sx1261_InitParam_WUR(sx_parameters, SYNCWORD);
            ESP_LOGI(TAG, "Sx1261_InitFonction");
            Sx1261_InitFonction_WUR(sx_parameters);
            ESP_LOGI(TAG, "Sx1261_WurMode");
            Sx1261_WurMode(sx_parameters, 999, 1);

            ESP_LOGW(TAG, "DIO1 Level after reset %d", gpio_get_level(CONFIG_DIO1_GPIO));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void parallel_wur_detection(void)
{
    gpio_config_t io_conf = {};
    rtc_gpio_hold_dis((gpio_num_t)CONFIG_NSS_GPIO);
    ESP_LOGI("INTERRUPTION", "Configuring SX1261");

    sx126x_reset(sx_parameters);
    ESP_LOGI("INTERRUPTION", "Sx1261_InitParamRX");
    Sx1261_InitParam_WUR(sx_parameters, SYNCWORD);
    ESP_LOGI("INTERRUPTION", "Sx1261_InitFonction");
    Sx1261_InitFonction_WUR(sx_parameters);
    ESP_LOGI("INTERRUPTION", "Sx1261_WurMode");

    xTaskCreatePinnedToCore(task_interruption, "task_interruption", 1024 * 4, NULL, 2, NULL, 1);
}
