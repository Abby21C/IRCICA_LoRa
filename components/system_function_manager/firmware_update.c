#include "firmware_update.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "utils.h"
#include "ota_update.h"
#include "http_ota_server.h"
#include "config.h"

static const char *TAG = "firmware_update";

void download_firmware_process(void)
{
    ESP_LOGI(TAG, "Initializing network stack...");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default Wi-Fi STA interface
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_config_sta = {
        .sta = {
            .ssid = WIFI_STA_SSID,
            .password = WIFI_STA_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "Waiting for IP...");
    vTaskDelay(pdMS_TO_TICKS(2000));

    print_sta_ip_address();

    const char *firmware_url = DEFAULT_FIRMWARE_URL;

    ESP_LOGI(TAG, "Downloading firmware from: %s", firmware_url);
    if (download_firmware_from_url(firmware_url) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to download firmware. OTA service not available.");
    } else {
        ESP_LOGI(TAG, "Firmware downloaded and ready for OTA.");
    }


}
