#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start OTA update from given URL.
 *        This function does not return on success—it will restart the device.
 *
 * @param url The HTTP or HTTPS URL pointing to the new firmware binary.
 */
void start_ota_update(const char *url);

#ifdef __cplusplus
}
#endif
