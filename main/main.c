#include <stdio.h> 
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h" 
#include "sx1261_WuR.h"
#include <stdint.h>   // C99 types
#include <stdbool.h>  // bool type
#include <driver/spi_master.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "sx126x.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_http_client.h"


#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "esp_netif_ip_addr.h"

#include "esp_ota_ops.h"

#include "ota_update.h"
#include "http_ota_server.h"
#include "payload_manager.h"
#include "async_web_server.h"
#include "led_status.h"
#include "wur_transmission.h"
#include "utils.h"

#include "config.h"

#include "system_setup.h"
#include "firmware_update.h"
#include "ota_server.h"
#include "wur_handler.h"


static const char *TAG = "example";

#define CONFIG_EXAMPLE_MAX_ASYNC_REQUESTS 3  // Set the maximum number of concurrent asynchronous requests
#define ASYNC_WORKER_STACK_SIZE 8192 



static uint8_t *downloaded_bin = NULL;
static size_t downloaded_bin_len = 0;



RTC_DATA_ATTR int number_of_wake_up = -1;

/**
 * exemple de code utilisant la librairie lorawan, et la wur
 * 
 * Pour émettre en wur, décommenter "#define TX_WUR"
 * Pour recevoir en wur, décommenter "#define RX_WUR"
 * 
 * Pour la librairie LoRaWAN (SWL2001-master), les fonctions utiles se trouvent 
 * dans components\SWL2001-master\smtc_modem_api\smtc_modem_api.h
 * la librairie doit d'abord être initialisée, puis ensuite la tache run_lora doit être lancée pour effectuer 
 * des appels réguliers à smtc_modem_run_engine qui va gérer les évênements de la librairie et faire tourner la 
 * machine à état de la librairie qui va faire le processus de connexion, d'envoi des trames etc
 * 
 * TODO: in smtc_modem_run_engine, we check irq each time, but we have to replace this by calling the callback
 * when an irq is triggered and not checking manualy the irq
*/


#define TAG "main" //for logs

//#define RX_WUR

#define TX_WUR

context_t sx_wur_param;
context_t *sx_parameters = &sx_wur_param;

//const char *firmware_url = DEFAULT_FIRMWARE_URL;



void app_main(void)
{
    setup_system();                 // Initialize GPIO, SPI, NVS, ISR, etc.
    download_firmware_process();   // Wi-Fi STA mode + firmware download
    start_ota_server();            // Switch to SoftAP + start OTA service

#ifdef TX_WUR
    start_wur_tx();                // Start WUR transmit mode
#endif

#ifdef RX_WUR
    start_wur_rx();                // Start WUR receive and enter Deep Sleep
#endif

    //Start the OTA update 
    //ESP_LOGI(TAG, "Starting OTA update using new ota_update module...");
    //start_ota_update(firmware_url);
    vTaskDelay(pdMS_TO_TICKS(50000));
    esp_restart();
    
}







/* --- EOF ------------------------------------------------------------------ */