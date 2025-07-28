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
#define SYNCWORD {0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae}

// ==== OTA Parameters ====
#define MAX_BIN_SIZE       (2 * 1024 * 1024)
#define DEFAULT_FIRMWARE_URL "http://192.168.22.200:8000/firmware.bin" //for the special node who need to connect to local AP
//#define DEFAULT_FIRMWARE_URL "http://192.168.4.1/firmware.bin"  //for each device in the network who can only connect to ESP32 peer's hotspot

// ==== WiFi ====
#define WIFI_STA_SSID      "POCOX6"  //for the special node
#define WIFI_STA_PASS      "22334455"  //for the special node
//#define WIFI_STA_SSID      "ESP32_HOTSPOT"  // for all device in the network except the special node
//#define WIFI_STA_PASS      "12345678"   // for all device in the network except the special node
#define WIFI_AP_SSID       "ESP32_HOTSPOT"
#define WIFI_AP_PASS       "12345678"
#define WIFI_AP_MAX_CONN   4  // Maximum number of connections to the ESP32 SoftAP, here set to 4 and leave one for the smartphone

// ==== Async Server ====
#define CONFIG_EXAMPLE_MAX_ASYNC_REQUESTS 3 // Set the maximum number of concurrent asynchronous requests

#endif
