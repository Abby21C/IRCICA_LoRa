#ifndef FIRMWARE_UPDATE_H
#define FIRMWARE_UPDATE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Connect to the network via Wi-Fi STA mode and download firmware from the configured URL.
 *
 * This function performs the following steps:
 * - Initialize network stack and event loop
 * - Start Wi-Fi in STA mode and connect to the configured AP
 * - Wait to obtain an IP address
 * - Call download_firmware_from_url() to download the firmware
 *
 * If the download fails, the system will log an error and skip OTA initialization.
 */
void download_firmware_process(void);

#ifdef __cplusplus
}
#endif

#endif // FIRMWARE_UPDATE_H
