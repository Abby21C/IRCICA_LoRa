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

#define STORAGE_NAMESPACE "ota_flag"
#define OTA_FLAG_KEY "is_updated"

#define OTA_RECEIVED_VERSION_KEY "rx_version" // Key for storing the received OTA version by payload



static uint8_t *downloaded_bin = NULL;
static size_t downloaded_bin_len = 0;



RTC_DATA_ATTR int number_of_wake_up = -1;


void reset_ota_flag() {    //only for development, to reset the OTA flag
    nvs_handle_t handle;
    esp_err_t err = nvs_open("ota_flag", NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        nvs_erase_key(handle, "is_updated");
        nvs_commit(handle);
        nvs_close(handle);
        ESP_LOGW("OTA", "OTA flag has been cleared");
    } else {
        ESP_LOGE("OTA", "Failed to open NVS, unable to clear OTA flag");
    }
}


int32_t get_local_ota_version() {  // Get the local OTA version from NVS
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return 0; 

    int32_t version = 0;
    nvs_get_i32(handle, OTA_FLAG_KEY, &version);
    nvs_close(handle);
    return version;
}


void set_local_ota_version(int32_t version) {  // Set the local OTA version in NVS
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return;

    nvs_set_i32(handle, OTA_FLAG_KEY, version); 
    nvs_commit(handle);
    nvs_close(handle);
}


void set_received_ota_version(int32_t version) {  // Set the received OTA version in NVS
    nvs_handle_t handle;
    if (nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_i32(handle, OTA_RECEIVED_VERSION_KEY, version);
        nvs_commit(handle);
        nvs_close(handle);
    }
}

int32_t get_received_ota_version_from_nvs() {  // Get the received OTA version from NVS
    nvs_handle_t handle;
    int32_t version = 0;
    if (nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_i32(handle, OTA_RECEIVED_VERSION_KEY, &version);
        nvs_close(handle);
    }
    return version;
}



/*
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

//#define TX_WUR

context_t sx_wur_param;
context_t *sx_parameters = &sx_wur_param;



void app_main(void) 
{
    
    setup_system();       // Initialize GPIO, SPI, NVS, ISR, etc

    //reset_ota_flag();   //only for development, to reset the OTA flag
    
    start_wur_rx();   //receive payload to get the newest OTA version

    // Validate the received payload format; if invalid, go back to deep sleep and wait for the next wake-up
    if (!is_payload_valid()) {  
        ESP_LOGE(TAG, "Payload invalid! Cannot proceed with OTA check.");
        esp_deep_sleep_start();
    }

    int32_t local_ota_version = get_local_ota_version(); //Read the current OTA version from NVS
    int32_t received_ota_version = (int32_t)get_received_ota_version();  // Read the received OTA version from payload
    ESP_LOGI(TAG, "Local OTA version: %ld", local_ota_version);
    ESP_LOGI(TAG, "Received OTA version: %ld", received_ota_version);

    //ESP_LOGW(TAG, "Going to deep sleep...");
    //esp_deep_sleep_start();  

    // Compare received OTA version with local version
    // If the received version is newer, attempt to download and apply the update
    if (received_ota_version > local_ota_version) {
        if (download_firmware_process()) {
            start_ota_server();  // Switch to SoftAP and start OTA service
            set_local_ota_version(received_ota_version);  // Save the new OTA version to NVS
            ESP_LOGI(TAG, "Writing new OTA version to NVS...");

            vTaskDelay(pdMS_TO_TICKS(60000)); 
            esp_restart(); //testing never shutdown
        } else {
            ESP_LOGE(TAG, "OTA already completed, skipping OTA process and entering normal business logic.");
            vTaskDelay(pdMS_TO_TICKS(3000));  // Wait for 3 seconds before proceeding
            esp_deep_sleep_start();  // If OTA failed, enter deep sleep waiting for next wake-up
        }

        //Main Tasks: here to put real task logic,for example, start the main application tasks
        //start_wur_rx();
        
    }
    

    #ifdef TX_WUR
        start_wur_tx();                // Start WUR transmit mode
    #endif

    #ifdef RX_WUR
        start_wur_rx();                // Start WUR receive and enter Deep Sleep
    #endif
    

}

//Sender, for testing purposes, the payload transmission feature can be integrated into the real project
/*void app_main(void)  
{
    
    setup_system();                 // Initialize GPIO, SPI, NVS, ISR, etc
    

#ifdef TX_WUR
    start_wur_tx();                // Start WUR transmit mode
#endif

#ifdef RX_WUR
    start_wur_rx();                // Start WUR receive and enter Deep Sleep
#endif

        //here to put real task logic,for example, start the main application tasks
        //start_wur_tx();
    

}*/







/* --- EOF ------------------------------------------------------------------ */