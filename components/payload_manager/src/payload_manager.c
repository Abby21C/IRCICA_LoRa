#include "payload_manager.h"
#include "sx1261_WuR.h"
#include "sx126x.h"
#include "esp_log.h"
#include "led_strip.h"

static const char* TAG = "PayloadMgr";

static payload_type_t last_received_type = PAYLOAD_TYPE_ALLINFO;

extern context_t* sx_parameters;
extern uint8_t SYNCWORD[];


// Payload data to be sent
static const uint8_t PAYLOAD_DATA_ALLINFO[] = {
    0x5E, 0xB3, 0x1C, 0x7A, 0xD9, 0x08, 0x92, 0x6F
};

static const uint8_t PAYLOAD_LED_WHITE[] = { PAYLOAD_TYPE_LED_WHITE };
static const uint8_t PAYLOAD_LED_RED[]   = { PAYLOAD_TYPE_LED_RED };
static const uint8_t PAYLOAD_LED_GREEN[] = { PAYLOAD_TYPE_LED_GREEN };


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
        default:
            ESP_LOGW(TAG, "Unknown payload type.");
            return;
    }

    ESP_LOGI(TAG, "Sending payload type %d, length %d", type, (int)length);
    Sx1261_Send_WuR_Signal(sx_parameters, payload, length);
}

// Receive and parse the payload
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

    payload_type_t received_type = (payload_type_t)received_payload[0];
    last_received_type = received_type;
    // Users can further process the received_type, e.g., control the LED based on type
}

payload_type_t get_last_received_payload_type(void) {
    return last_received_type;
}
