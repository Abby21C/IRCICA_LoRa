#ifndef PAYLOAD_MANAGER_H
#define PAYLOAD_MANAGER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Payload type definition
typedef enum {
    PAYLOAD_TYPE_ALLINFO = 0,
    PAYLOAD_TYPE_LED_WHITE = 1,
    PAYLOAD_TYPE_LED_RED   = 2,
    PAYLOAD_TYPE_LED_GREEN = 3
} payload_type_t;


// Send a payload
void send_payload(payload_type_t type);

// Receive and process a payload
void receive_payload(void);

payload_type_t get_last_received_payload_type(void);

#ifdef __cplusplus
}
#endif

#endif // PAYLOAD_MANAGER_H
