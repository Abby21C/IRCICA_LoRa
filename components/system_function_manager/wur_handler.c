#include "wur_handler.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "driver/gpio.h"
#include "sx1261_WuR.h"
#include "config.h"
#include "wur_transmission.h"


static const char *TAG = "wur_handler";
extern int number_of_wake_up;

// Global context parameters (can be externally provided if needed)
//context_t sx_wur_param;
//context_t *sx_parameters = &sx_wur_param;
extern context_t *sx_parameters;


void start_wur_tx(void)
{
    ESP_LOGI(TAG, "Initializing SX1261 for TX...");
    sx1261_init();

    ESP_LOGI(TAG, "Starting WUR TX...");
    wur_start_tx(sx_parameters);
}

void start_wur_rx(void)
{
    ESP_LOGW(TAG, "WOKE UP FROM DEEP SLEEP");

    sx1261_init();

    // Start one-shot reception (power saving)
    wur_rx_once(sx_parameters);

    // Show wake-up count or level status
    number_of_wake_up++;
    ESP_LOGW(TAG, "DIO1 Level on Wake-up: %d", gpio_get_level(CONFIG_DIO1_GPIO));

    // Configure DIO1 as an external wake-up source
    uint64_t ext_wakeup_pins_mask = (1ULL << CONFIG_DIO1_GPIO);

    rtc_gpio_set_level(CONFIG_NSS_GPIO, 1);
    rtc_gpio_pulldown_en(CONFIG_DIO1_GPIO);
    rtc_gpio_hold_en(CONFIG_NSS_GPIO);

    esp_sleep_enable_ext1_wakeup(ext_wakeup_pins_mask, ESP_EXT1_WAKEUP_ANY_HIGH);

    //ESP_LOGW(TAG, "Going to deep sleep..."); // Uncomment to enable deep sleep
    //esp_deep_sleep_start();   
}