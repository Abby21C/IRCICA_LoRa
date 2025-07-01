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



static const char *TAG = "example";

#define CONFIG_EXAMPLE_MAX_ASYNC_REQUESTS 3  // Set the maximum number of concurrent asynchronous requests
#define ASYNC_WORKER_STACK_SIZE 8192



/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 48
#define CONFIG_NSS_GPIO 7
#define CONFIG_RST_GPIO 1
#define CONFIG_BUSY_GPIO 5
#define CONFIG_DIO1_GPIO 6
#define CONFIG_TXEN_GPIO -1
#define CONFIG_RXEN_GPIO -1

uint8_t SYNCWORD[] = {0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae};



#define MAX_BIN_SIZE (2 * 1024 * 1024)  // 2MB max firmware
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



void print_sta_ip_address(void)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    if (netif == NULL) {
        ESP_LOGE("IP_CHECK", "Failed to get netif handle for STA.");
        return;
    }

    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        ESP_LOGI("IP_CHECK", "ESP32 IP Address: " IPSTR, IP2STR(&ip_info.ip));
        ESP_LOGI("IP_CHECK", "Netmask: " IPSTR, IP2STR(&ip_info.netmask));
        ESP_LOGI("IP_CHECK", "Gateway: " IPSTR, IP2STR(&ip_info.gw));
    } else {
        ESP_LOGE("IP_CHECK", "Failed to get IP info");
    }
}



/**
 * @brief simple task that send a WuB each 3 secondes
 * 
 */
void run_TX_WUR(void *pvParameters)
{
    uint64_t exec_time = 0;
    uint8_t compteur = 0;
    
    while (1)
    {
        
        
        send_payload(PAYLOAD_TYPE_LED_GREEN);

        /*static const uint8_t PAYLOAD_LED_RED[] = { PAYLOAD_TYPE_LED_RED };
        send_payload(PAYLOAD_TYPE_LED_RED);
        */

        compteur++;
        ESP_LOGI(TAG, "number of signal sended: %d", compteur);
        vTaskDelay(pdMS_TO_TICKS(10000));
	}
}

/**
 * @brief simple task that print the number of time that the iot woke up when he wakes up
 * 
 */
void run_RX_WUR(void *pvParameters)
{
    int nb_reveils = 0;
    uint64_t init_time = esp_timer_get_time();; //heure au début à l'initialisation
    uint64_t current_time = 0; //heure courante
    sx126x_irq_mask_t sx126x_irq_mask;
    ESP_LOGW(TAG, "DIO1 Level %d", gpio_get_level(CONFIG_DIO1_GPIO));
    while (1)
    {
        //we check irq status (note : it has to be coded with interrupt esp)
        sx126x_get_irq_status(sx_parameters,&sx126x_irq_mask);
        if (sx126x_irq_mask & SX126X_IRQ_SYNC_WORD_VALID)
        {
            nb_reveils ++;
			current_time = esp_timer_get_time();
            ESP_LOGW(TAG, "irq : %d", sx126x_irq_mask);
            ESP_LOGW(TAG, "DIO1 Level %d", gpio_get_level(CONFIG_DIO1_GPIO));
			ESP_LOGI(TAG, "reveil, nb_reveils : %d ;  temps écoulé : %llu s ",
			nb_reveils, (current_time - init_time)/1000000);

            
            led_status_toggle();
            vTaskDelay(pdMS_TO_TICKS(1000));
            led_status_toggle();

			
        	//vTaskDelay(pdMS_TO_TICKS(1000));
			//on repasse en sommeil

        
        
        receive_payload();

        
            ESP_LOGI(TAG, "Sx1261_WurMode");
            sx126x_clear_irq_status( sx_parameters, &sx126x_irq_mask );
            sx126x_reset(sx_parameters);
            ESP_LOGI(TAG, "Sx1261_InitParamRX");
            Sx1261_InitParam_WUR(sx_parameters, SYNCWORD);
            ESP_LOGI(TAG, "Sx1261_InitFonction");
            Sx1261_InitFonction_WUR(sx_parameters);

            
			ESP_LOGI(TAG, "Sx1261_WurMode");
            sx126x_clear_irq_status( sx_parameters, &sx126x_irq_mask );
            sx126x_reset(sx_parameters);
            ESP_LOGI(TAG, "Sx1261_InitParamRX");
            Sx1261_InitParam_WUR(sx_parameters, SYNCWORD);
            ESP_LOGI(TAG, "Sx1261_InitFonction");
            Sx1261_InitFonction_WUR(sx_parameters);
            ESP_LOGI(TAG, "Sx1261_WurMode");
            Sx1261_WurMode(sx_parameters, 999, 1);
            //ESP_LOGI(TAG, "Sx1261_WurMode");
            ESP_LOGW(TAG, "DIO1 Level after reset %d", gpio_get_level(CONFIG_DIO1_GPIO));
            //Sx1261_WurMode(sx_parameters, 999, 1);

       
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task_interruption( void * parameter )
{   //TODO - activate the interruption in esp32, and dont do the for loop
    sx126x_irq_mask_t sx126x_irq_mask;
    ESP_LOGW(TAG, "DIO1 Level %d", gpio_get_level(CONFIG_DIO1_GPIO));

    while (true)
    {
        //we check irq status (note : it has to be coded with interrupt esp)
        sx126x_get_irq_status(sx_parameters,&sx126x_irq_mask);
        if (sx126x_irq_mask & SX126X_IRQ_SYNC_WORD_VALID)
        {
            ESP_LOGW(TAG, "irq : %d", sx126x_irq_mask);
            ESP_LOGW(TAG, "DIO1 Level %d", gpio_get_level(CONFIG_DIO1_GPIO));
            ESP_LOGE(TAG, "WOKE UP IN TASK")   ;
			
        	vTaskDelay(pdMS_TO_TICKS(1000));
			//on repasse en sommeil
			ESP_LOGI(TAG, "Sx1261_WurMode");
            sx126x_clear_irq_status( sx_parameters, &sx126x_irq_mask );
            sx126x_reset(sx_parameters);
            ESP_LOGI(TAG, "Sx1261_InitParamRX");
            Sx1261_InitParam_WUR(sx_parameters, SYNCWORD);
            ESP_LOGI(TAG, "Sx1261_InitFonction");
            Sx1261_InitFonction_WUR(sx_parameters);
            ESP_LOGI(TAG, "Sx1261_WurMode");
            Sx1261_WurMode(sx_parameters, 999, 1);
            //ESP_LOGI(TAG, "Sx1261_WurMode");
            ESP_LOGW(TAG, "DIO1 Level after reset %d", gpio_get_level(CONFIG_DIO1_GPIO));
            //Sx1261_WurMode(sx_parameters, 999, 1);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void parallel_wur_detection(){
    gpio_config_t io_conf = {};
    rtc_gpio_hold_dis((gpio_num_t)CONFIG_NSS_GPIO);
    ESP_LOGI("INTERRUPTION", "\033[0mConfiguring SX1261");
    sx126x_reset(sx_parameters);
    ESP_LOGI("INTERRUPTION", "Sx1261_InitParamRX");
    Sx1261_InitParam_WUR(sx_parameters, SYNCWORD);
    ESP_LOGI("INTERRUPTION", "Sx1261_InitFonction");
    Sx1261_InitFonction_WUR(sx_parameters);
    ESP_LOGI("INTERRUPTION", "Sx1261_WurMode");

    // Start the task to handle shocks
    xTaskCreatePinnedToCore(task_interruption, "task_interruption", 1024 * 4, NULL, 2, NULL, 1);
}

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

    const char *firmware_url = "http://192.168.83.200:8000/firmware.bin";
    if (download_firmware_from_url(firmware_url) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to download firmware. OTA service not available.");
    } else {
        ESP_LOGI(TAG, "Firmware ready for OTA");
    

    /* Start the OTA update 
    ESP_LOGI(TAG, "Starting OTA update using new ota_update module...");
    start_ota_update(firmware_url);
    */

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
        sx1261_init();
        sx126x_reset(sx_parameters);
        ESP_LOGI(TAG, "initparam");
        Sx1261_InitParam_WUR(sx_parameters, SYNCWORD);
        ESP_LOGI(TAG, "initfonction");
        Sx1261_InitFonction_WUR(sx_parameters);
	    xTaskCreate(&run_TX_WUR, "run_TX_WUR", 1024*4, NULL, 5, NULL);
    #endif
    #ifdef RX_WUR
        ESP_LOGW(TAG, "WOKE UP DEEP SLEEP");
        sx1261_init();
        receive_payload();

        spi_device_interface_config_t spiConfig;
        memset(&spiConfig, 0, sizeof(spiConfig));
        spiConfig.mode = 0;
        spiConfig.clock_speed_hz = 50000;
        spiConfig.command_bits = 8;
        spiConfig.address_bits = 8;
        spiConfig.spics_io_num = 4;
        spiConfig.queue_size = 1;
        spiConfig.cs_ena_posttrans = 2;
        spi_device_handle_t spiHandle_accel;
        esp_err_t ret = spi_bus_add_device(SPI2_HOST, &spiConfig, &spiHandle_accel);
        number_of_wake_up++;

        ESP_LOGW(TAG, "DIO1 Level wake up %d", gpio_get_level(CONFIG_DIO1_GPIO));
        
        sx126x_reset(sx_parameters);
        ESP_LOGI(TAG, "Sx1261_InitParamRX");
        Sx1261_InitParam_WUR(sx_parameters, SYNCWORD);
        ESP_LOGI(TAG, "Sx1261_InitFonction");
        Sx1261_InitFonction_WUR(sx_parameters);
        ESP_LOGI(TAG, "Sx1261_WurMode");
        Sx1261_WurMode(sx_parameters, 999, 1);
        ESP_LOGI("INTERRUPTION", "\033[0mWuR interrupt enable");
        //vTaskDelay(pdMS_TO_TICKS(1000));

        //xTaskCreate(&run_RX_WUR, "run_RX_WUR", 1024*4, NULL, 5, NULL);
        
        uint64_t ext_wakeup_pins_mask = (1ULL << CONFIG_DIO1_GPIO);
        
        
        rtc_gpio_set_level(CONFIG_NSS_GPIO,1);
        rtc_gpio_pulldown_en(CONFIG_DIO1_GPIO);
        rtc_gpio_hold_en(CONFIG_NSS_GPIO);
        //vTaskDelay(pdMS_TO_TICKS(1000));
    
        esp_sleep_enable_ext1_wakeup(ext_wakeup_pins_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
        ESP_LOGW(TAG, "GOING TO DEEP SLEEP");
        esp_deep_sleep_start();
        


    #endif

}






/* --- EOF ------------------------------------------------------------------ */