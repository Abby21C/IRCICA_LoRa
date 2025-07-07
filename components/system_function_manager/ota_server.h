#ifndef OTA_SERVER_H
#define OTA_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the OTA file server.
 *
 * This function performs the following operations:
 * - Stop Wi-Fi STA mode
 * - Configure and start SoftAP (Access Point) mode
 * - Initialize asynchronous web server
 * - Start OTA firmware file service (allows other devices to download firmware.bin)
 *
 * Note: This function should be called after the firmware has been successfully downloaded.
 */
void start_ota_server(void);

#ifdef __cplusplus
}
#endif

#endif // OTA_SERVER_H
