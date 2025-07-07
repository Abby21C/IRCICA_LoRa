#include "ota_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "string.h"

#include "async_web_server.h"
#include "http_ota_server.h"
#include "config.h"

static const char *TAG = "ota_server";

void start_ota_server(void)
{
    ESP_LOGI(TAG, "Stopping STA and switching to SoftAP mode...");

    // Stop STA mode
    ESP_ERROR_CHECK(esp_wifi_stop());

    // Create default SoftAP interface
    esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .password = WIFI_AP_PASS,
            .max_connection = WIFI_AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    // If password is empty, set to open mode
    if (wifi_config_ap.ap.password[0] == '\0') {
        wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP started. SSID: %s, Password: %s",
             wifi_config_ap.ap.ssid, wifi_config_ap.ap.password);

    // Start asynchronous Web OTA file service
    async_web_server_init(CONFIG_EXAMPLE_MAX_ASYNC_REQUESTS);
    start_ota_file_server();

    ESP_LOGI(TAG, "OTA file server started.");
}
