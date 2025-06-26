#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the OTA HTTP service and register the URI (/firmware.bin)
 *
 * Initializes the HTTP server and registers a URI handler for firmware distribution.
 *
 * @return
 *      - ESP_OK: Service started and URI registered successfully
 *      - ESP_FAIL: Failed to start service or register URI
 */
esp_err_t start_ota_file_server(void);

/**
 * @brief Download OTA firmware from a specified URL and write to the update partition
 *
 * Uses the ESP HTTP client to download the firmware binary and writes it to the target OTA partition via esp_ota.
 * Note: The device will not reboot immediately after the download completes.
 *
 * @param url Full HTTP URL of the firmware .bin file
 * @return
 *      - ESP_OK: Download and write successful
 *      - ESP_FAIL: Error occurred during download or write
 */
esp_err_t download_firmware_from_url(const char *url);

#ifdef __cplusplus
}
#endif