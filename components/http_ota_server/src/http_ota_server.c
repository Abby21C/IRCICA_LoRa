#include "http_ota_server.h"
#include "esp_http_server.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "async_web_server.h"

static const char *TAG = "http_ota_server";

/**
 * @brief http GET handler for serving the firmware binary file
 */
static esp_err_t firmware_get_handler(httpd_req_t *req)
{
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    if (!partition) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA partition not found");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/octet-stream");

    const size_t chunk_size = 1024;
    uint8_t *buffer = malloc(chunk_size);
    if (!buffer) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Buffer alloc failed");
        return ESP_FAIL;
    }

    size_t offset = 0;
    size_t remaining = partition->size;
    esp_err_t err;
    size_t total_sent = 0;

    while (remaining > 0) {
        size_t to_read = remaining > chunk_size ? chunk_size : remaining;
        err = esp_partition_read(partition, offset, buffer, to_read);
        if (err != ESP_OK) {
            free(buffer);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Read failed");
            return ESP_FAIL;
        }

        err = httpd_resp_send_chunk(req, (const char *)buffer, to_read);
        if (err != ESP_OK) {
            free(buffer);
            return err;
        }

        offset += to_read;
        remaining -= to_read;
        total_sent += to_read;
        if ((total_sent % (64 * 1024)) == 0) {
            ESP_LOGI(TAG, "Sent %d KB to client", total_sent / 1024);
        }
    }

    httpd_resp_send_chunk(req, NULL, 0);
    free(buffer);
    return ESP_OK;
}

static esp_err_t firmware_async_entry(httpd_req_t *req)
{
    return async_web_server_submit(req, firmware_get_handler);
}

/**
 * @brief start the OTA file server
 */
esp_err_t start_ota_file_server(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ret;
    }

    httpd_uri_t firmware_uri = {
    .uri      = "/firmware.bin",
    .method   = HTTP_GET,
    .handler  = firmware_async_entry,
    .user_ctx = NULL
};

    return httpd_register_uri_handler(server, &firmware_uri);
}


/**
 * @brief download firmware from a given URL and perform OTA update
 */
esp_err_t download_firmware_from_url(const char *url)
{
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (esp_http_client_open(client, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection");
        return ESP_FAIL;
    }

    esp_http_client_fetch_headers(client);

    const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) {
        ESP_LOGE(TAG, "OTA partition not found");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    esp_ota_handle_t ota_handle;
    if (esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    char buffer[1024];
    int data_read;
    int total_bytes = 0;

    while ((data_read = esp_http_client_read(client, buffer, sizeof(buffer))) > 0) {
        total_bytes += data_read;
        static bool first_chunk_logged = false;
        if (!first_chunk_logged && data_read >= 4) {
            ESP_LOGI(TAG, "First bytes: 0x%02X 0x%02X 0x%02X 0x%02X",
                     buffer[0], buffer[1], buffer[2], buffer[3]);
            first_chunk_logged = true;
        }

        if (esp_ota_write(ota_handle, buffer, data_read) != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed");
            esp_http_client_cleanup(client);
            esp_ota_end(ota_handle);
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Total OTA bytes read: %d", total_bytes);

    if (data_read < 0) {
        ESP_LOGE(TAG, "esp_http_client_read error");
        esp_http_client_cleanup(client);
        esp_ota_end(ota_handle);
        return ESP_FAIL;
    }

    if (esp_ota_end(ota_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    if (esp_ota_set_boot_partition(ota_partition) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "OTA update successful (no reboot yet)");
    esp_http_client_cleanup(client);
    return ESP_OK;
}