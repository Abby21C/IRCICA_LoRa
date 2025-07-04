#include <stdio.h> 
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h" //FIXME - check if all of these includes are needed
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


static const char *TAG = "example";

#define CONFIG_EXAMPLE_MAX_ASYNC_REQUESTS 3  // Set the maximum number of concurrent asynchronous requests
#define ASYNC_WORKER_STACK_SIZE 8192 //FIXME - this one can go



/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 48 //FIXME - put this definitions in the config file
#define CONFIG_NSS_GPIO 7 //FIXME - you have this definition in 3 different files, put it in a config file
#define CONFIG_RST_GPIO 1 //FIXME - same as above
#define CONFIG_BUSY_GPIO 5
#define CONFIG_DIO1_GPIO 6
#define CONFIG_TXEN_GPIO -1
#define CONFIG_RXEN_GPIO -1 //FIXME - fix all this definitios

uint8_t SYNCWORD[] = {0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae}; //FIXME - same as above, if youre defining the same variable in multiple files, put it in a config file



#define MAX_BIN_SIZE (2 * 1024 * 1024)  // 2MB max firmware //FIXME - could be in the config file
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




void app_main(void)
{
    rtc_gpio_hold_dis(CONFIG_NSS_GPIO);
    
    led_status_init(BLINK_GPIO);
    led_status_toggle();
    vTaskDelay(pdMS_TO_TICKS(1000));
    led_status_toggle();


	//initialize radio modem with context
	sx126x_hal_context_t* context;
    context = malloc(sizeof(sx126x_hal_context_t));
    
    context->nss = (uint8_t) CONFIG_NSS_GPIO;
    context->busy = (uint8_t) CONFIG_BUSY_GPIO;
    context->reset = (uint8_t) CONFIG_RST_GPIO;
    context->spi_id = (uint32_t) SPI2_HOST;

	LoRaDebugPrint(true);

    esp_err_t err;

    // Initialize the GPIO ISR handler service
    err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    ESP_ERROR_CHECK(err);




    // Initialize NVS
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    //FIXME - from the beggining of the main until here, you can put it in a setup function

    //FIXME - from here until it download the firmware, you can put it in a function (could be called DownloadFirmware)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_config_sta = {
        .sta = {
            .ssid = "POCOX6",   //input Wi-Fi SSID
            .password = "22334455",  //input Wi-Fi password
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    vTaskDelay(pdMS_TO_TICKS(2000));
    print_sta_ip_address();  //Print ESP ip

    const char *firmware_url = "http://192.168.22.200:8000/firmware.bin"; //FIXME - could be in the config file
    if (download_firmware_from_url(firmware_url) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to download firmware. OTA service not available.");
    } else {
        ESP_LOGI(TAG, "Firmware ready for OTA");
    

    /* Start the OTA update 
    ESP_LOGI(TAG, "Starting OTA update using new ota_update module...");
    start_ota_update(firmware_url);
    */
    //FIXME - From here until you start the server, you can put it in a function (could be called StartServer)
    ESP_ERROR_CHECK(esp_wifi_stop());

    esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t wifi_config_ap = {
            .ap = {
                .ssid = "ESP32_HOTSPOT",
                .ssid_len = strlen("ESP32_HOTSPOT"),
                .password = "12345678",
                .max_connection = 4,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK
            },
        };
    if (wifi_config_ap.ap.password[0] == '\0') {
        wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "SoftAP started. SSID:%s password:%s", wifi_config_ap.ap.ssid, wifi_config_ap.ap.password);

    //start_async_req_workers();
    async_web_server_init(CONFIG_EXAMPLE_MAX_ASYNC_REQUESTS);
    start_ota_file_server();

    }


    //sx1261_init();
    /*  PARTIE WUR*/
    #ifdef TX_WUR
        //FIXME - TX can be also a function
        sx1261_init();
        
        wur_start_tx(sx_parameters);

    #endif
    #ifdef RX_WUR
    //FIXME - Can be a function too.
    ESP_LOGW(TAG, "WOKE UP DEEP SLEEP");
    sx1261_init(); 

    wur_rx_once(sx_parameters);  // Start RX once after wake-up in low-power mode

    number_of_wake_up++;
    ESP_LOGW(TAG, "DIO1 Level wake up %d", gpio_get_level(CONFIG_DIO1_GPIO));

    
    // Configure DIO1 as the wake-up pin
    uint64_t ext_wakeup_pins_mask = (1ULL << CONFIG_DIO1_GPIO);
    rtc_gpio_set_level(CONFIG_NSS_GPIO, 1);
    rtc_gpio_pulldown_en(CONFIG_DIO1_GPIO);
    rtc_gpio_hold_en(CONFIG_NSS_GPIO);

    esp_sleep_enable_ext1_wakeup(ext_wakeup_pins_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
    ESP_LOGW(TAG, "GOING TO DEEP SLEEP");
    esp_deep_sleep_start();
#endif

//FIXME - If you put everything I just told you in functions, your main will have 4 lines
/*
app_main(){

    setup(); //Initialize NVS, Wi-Fi, etc.
    DownloadFirmware(); //Download the firmware
    StartServer(); //Start the server and OTA service
    if (TX_WUR) {
        start_wur_tx(); //Start WUR transmission
    }
    if (RX_WUR) {
        start_wur_rx(); //Start WUR reception
    }
}
    
*/

}






/* --- EOF ------------------------------------------------------------------ */