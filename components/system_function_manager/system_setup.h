#ifndef SYSTEM_SETUP_H
#define SYSTEM_SETUP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize system environment, including:
 * - Release NSS pin from hold state
 * - Initialize LED indicator
 * - Configure SPI context for SX126x
 * - Install GPIO interrupt service
 * - Initialize NVS storage
 */
void setup_system(void);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_SETUP_H
