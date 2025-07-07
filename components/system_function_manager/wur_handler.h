#ifndef WUR_HANDLER_H
#define WUR_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start Wake-Up Radio transmission task (TX mode)
 */
void start_wur_tx(void);

/**
 * @brief Start Wake-Up Radio reception task (RX mode), suitable for low-power mode
 */
void start_wur_rx(void);


#ifdef __cplusplus
}
#endif

#endif // WUR_HANDLER_H
