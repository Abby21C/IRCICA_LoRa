#include "system_setup.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "sx126x.h"
#include "led_status.h"
#include "driver/rtc_io.h"
#include "ra01s.h"
#include "config.h"

static const char *TAG = "system_setup";

void setup_system(void)
{
    // Disable hold on NSS pin
    rtc_gpio_hold_dis(CONFIG_NSS_GPIO);

    // Initialize LED status indicator
    led_status_init(BLINK_GPIO);
    led_status_toggle();
    vTaskDelay(pdMS_TO_TICKS(1000));
    led_status_toggle();

    // Initialize SPI control structure (if needed)
    sx126x_hal_context_t* context = malloc(sizeof(sx126x_hal_context_t));
    context->nss = (uint8_t) CONFIG_NSS_GPIO;
    context->busy = (uint8_t) CONFIG_BUSY_GPIO;
    context->reset = (uint8_t) CONFIG_RST_GPIO;
    context->spi_id = (uint32_t) SPI2_HOST;

    LoRaDebugPrint(true);  // Enable LoRa debug output

    // Install GPIO interrupt service
    esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    ESP_ERROR_CHECK(err);

    // Initialize NVS (used for Wi-Fi or parameter storage)
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "System setup completed.");
}
