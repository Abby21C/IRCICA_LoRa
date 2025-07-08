#include "ota_update.h"

#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_log.h"
#include <string.h>
#include <errno.h>

#include "config.h"

#define BUFFSIZE 1024
static const char *TAG = "ota_update";

static char ota_write_data[BUFFSIZE + 1] = { 0 };

static void http_cleanup(esp_http_client_handle_t client) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void task_fatal_error(void) {
    ESP_LOGE(TAG, "OTA failed. Stopping task.");
    vTaskDelete(NULL);
    while (1) { ; }
}

void start_ota_update(const char *url) {
    esp_err_t err;
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA from URL: %s", url);

    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 5000,
        .keep_alive_enable = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP connection");
        task_fatal_error();
    }
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }

    esp_http_client_fetch_headers(client);

    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition at offset 0x%" PRIx32, update_partition->address);

    bool image_header_checked = false;
    int binary_file_length = 0;

    while (1) {
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "SSL read error");
            http_cleanup(client);
            task_fatal_error();
        } else if (data_read > 0) {
            if (!image_header_checked) {
                err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
                    http_cleanup(client);
                    task_fatal_error();
                }
                ESP_LOGI(TAG, "esp_ota_begin succeeded");
                image_header_checked = true;
            }

            err = esp_ota_write(update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
                http_cleanup(client);
                task_fatal_error();
            }

            binary_file_length += data_read;
        } else if (data_read == 0) {
            if (esp_http_client_is_complete_data_received(client)) {
                ESP_LOGI(TAG, "OTA download completed.");
                break;
            }
        }
    }

    ESP_LOGI(TAG, "Total image size: %d bytes", binary_file_length);

    if ((err = esp_ota_end(update_handle)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }

    if ((err = esp_ota_set_boot_partition(update_partition)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }

    ESP_LOGI(TAG, "OTA update successful. Rebooting...");
    esp_restart();
}
