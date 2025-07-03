// wur_transmission.h

#pragma once

#include "sx126x.h"  // for context_t

#ifdef __cplusplus
extern "C" {
#endif

void wur_start_tx(context_t *external_context); 
void wur_start_rx(context_t *external_context);  // Start RX mode for continuous listening
void wur_rx_once(context_t *external_context); //Start RX once after wake-up in low-power mode


#ifdef __cplusplus
}
#endif
