#include "payload_manager.h"
#include "sx1261_WuR.h"
#include "sx126x.h"
#include "esp_log.h"
#include "led_strip.h"
#include "led_status.h"

#include "config.h"

static const char* TAG = "PayloadMgr";


extern context_t* sx_parameters;
//extern const uint8_t SYNCWORD[];
//static const uint8_t syncword[SYNCWORD_LEN] = SYNCWORD;


#define PAYLOAD_TYPE_OTA_VERSION 0x04

// Payload data to be sent
static const uint8_t PAYLOAD_DATA_ALLINFO[] = {
    0x5E, 0xB3, 0x1C, 0x7A, 0xD9, 0x08, 0x92, 0x6F
};

static const uint8_t PAYLOAD_LED_WHITE[] = { PAYLOAD_TYPE_LED_WHITE };
static const uint8_t PAYLOAD_LED_RED[]   = { PAYLOAD_TYPE_LED_RED };
static const uint8_t PAYLOAD_LED_GREEN[] = { PAYLOAD_TYPE_LED_GREEN };

static uint8_t latest_received_version = 0;

static bool is_valid_payload = false;

bool is_payload_valid(void) {
    return is_valid_payload;
}

//Inform the OTA version to the receiver
void send_ota_version_payload(uint8_t version) {
    uint8_t payload[2];
    payload[0] = PAYLOAD_TYPE_OTA_VERSION;
    payload[1] = version;  // OTA version number

    ESP_LOGI(TAG, "Sending OTA Version payload, version: %d", version);
    Sx1261_Send_WuR_Signal(sx_parameters, payload, sizeof(payload));
}


// Send a payload
void send_payload(payload_type_t type) {
    const uint8_t* payload = NULL;
    size_t length = 0;

    switch (type) {
        case PAYLOAD_TYPE_ALLINFO:
            payload = PAYLOAD_DATA_ALLINFO;
            length = sizeof(PAYLOAD_DATA_ALLINFO);
            break;
        case PAYLOAD_TYPE_LED_WHITE:
            payload = PAYLOAD_LED_WHITE;
            length = sizeof(PAYLOAD_LED_WHITE);
            break;
        case PAYLOAD_TYPE_LED_RED:
            payload = PAYLOAD_LED_RED;
            length = sizeof(PAYLOAD_LED_RED);
            break;
        case PAYLOAD_TYPE_LED_GREEN:
            payload = PAYLOAD_LED_GREEN;
            length = sizeof(PAYLOAD_LED_GREEN);
            break;
        case PAYLOAD_TYPE_OTA_VERSION:
            send_ota_version_payload(14); 
            return;
        default:
            ESP_LOGW(TAG, "Unknown payload type.");
            return;
    }

    ESP_LOGI(TAG, "Sending payload type %d, length %d", type, (int)length);
    Sx1261_Send_WuR_Signal(sx_parameters, payload, length);
}


void receive_payload(void) {
    sx126x_rx_buffer_status_t rx_status;
    uint8_t received_payload[256] = {0};

    sx126x_get_rx_buffer_status(sx_parameters, &rx_status);
    sx126x_read_buffer(sx_parameters, rx_status.buffer_start_pointer,
                       received_payload, rx_status.pld_len_in_bytes);

    ESP_LOGI(TAG, "Received payload (%d bytes):", rx_status.pld_len_in_bytes);
    for (int i = 0; i < rx_status.pld_len_in_bytes; i++) {
        ESP_LOGI(TAG, "Byte[%d] = 0x%02X", i, received_payload[i]);
    }

    /*if (rx_status.pld_len_in_bytes < 1) {
        ESP_LOGW(TAG, "Received empty payload.");
        return;
    }*/

    if (rx_status.pld_len_in_bytes < 1) {
    ESP_LOGW(TAG, "Received empty payload.");
    is_valid_payload = false;
    return;
    }

    is_valid_payload = true;

    payload_type_t received_type = (payload_type_t)received_payload[0];

    if (received_type == PAYLOAD_TYPE_OTA_VERSION && rx_status.pld_len_in_bytes >= 2) {
        uint8_t version = received_payload[1];
        latest_received_version = version; // Store the received version
        ESP_LOGI(TAG, "Received OTA version: %d", version);
        
    } else {
        led_status_handle_payload(received_type);
    }
}

uint8_t get_received_ota_version(void) {
    return latest_received_version;
}
