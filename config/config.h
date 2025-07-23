#ifndef CONFIG_H
#define CONFIG_H

// ==== GPIO Pins ====
#define BLINK_GPIO         48
#define CONFIG_NSS_GPIO    7
#define CONFIG_RST_GPIO    1
#define CONFIG_BUSY_GPIO   5
#define CONFIG_DIO1_GPIO   6
#define CONFIG_TXEN_GPIO   -1
#define CONFIG_RXEN_GPIO   -1

// ==== SYNCWORD ====
#define SYNCWORD_LEN 8
//static const uint8_t SYNCWORD[SYNCWORD_LEN] = {0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae};
#define SYNCWORD {0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae}

// ==== OTA Parameters ====
#define MAX_BIN_SIZE       (2 * 1024 * 1024)
//#define DEFAULT_FIRMWARE_URL "http://192.168.22.200:8000/firmware.bin"
#define DEFAULT_FIRMWARE_URL "http://192.168.4.1/firmware.bin"

// ==== WiFi ====
//#define WIFI_STA_SSID      "POCOX6"
//#define WIFI_STA_PASS      "22334455"
#define WIFI_STA_SSID      "ESP32_HOTSPOT"
#define WIFI_STA_PASS      "12345678"
#define WIFI_AP_SSID       "ESP32_HOTSPOT"
#define WIFI_AP_PASS       "12345678"
#define WIFI_AP_MAX_CONN   4

// ==== Async Server ====
#define CONFIG_EXAMPLE_MAX_ASYNC_REQUESTS 3

#endif
