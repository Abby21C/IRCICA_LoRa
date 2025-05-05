
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

#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "example";

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

static uint8_t s_led_state = 0;

static led_strip_handle_t led_strip;

static void blink_led(void)
{
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    } else {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}

static void show_color_led(uint8_t color_code)
{
    switch (color_code) {
        case 1: // WHITE
            led_strip_set_pixel(led_strip, 0, 16, 16, 16);
            break;
        case 2: // RED
            led_strip_set_pixel(led_strip, 0, 48, 0, 0);
            break;
        case 3: // GREEN
            led_strip_set_pixel(led_strip, 0, 0, 48, 0);
            break;
        default:
            led_strip_clear(led_strip);
            return;
    }
    led_strip_refresh(led_strip);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };

    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI3_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

typedef enum {
    PAYLOAD_TYPE_ALLINFO = 0,
    PAYLOAD_TYPE_LED_WHITE  = 1,
    PAYLOAD_TYPE_LED_RED    = 2,
    PAYLOAD_TYPE_LED_GREEN  = 3
} payload_type_t;

/*static const uint8_t PAYLOAD_DATA_ALLINFO[] = {
    0x06, 0x8B, 0x3F, 0x42, 0x36, 0x4F,
    0x00, 0x5A, 0x8B, 0x21, 0x11, 0x2C
};
*/


static const uint8_t PAYLOAD_DATA_ALLINFO[] = {
    0x5E, 0xB3, 0x1C, 0x7A, 0xD9, 0x04,
    0x92, 0x6F, 0x38, 0xAC, 0x0D, 0xE1,
    0x53, 0xCF, 0x70, 0x18, 0xA6, 0x25,
    0xF4, 0x3B, 0x81, 0xD0, 0x6A, 0x19,
    0xBE, 0x42, 0x8D, 0x09, 0x77, 0xEC,
    0x34, 0x9B, 0x0A, 0xC1, 0x5D, 0x68,
    0xAF, 0x12, 0x93, 0xFE, 0x60, 0x37,
    0xD2, 0x84, 0x1A, 0xB7, 0x4F, 0x00,
    0xEE, 0x29, 0x7D, 0xC6, 0x10, 0xAB,
    0x3E, 0xF8, 0x05, 0x9D, 0x61, 0x2C,
    0x8A, 0xBD, 0x47, 0x31, 0xFA, 0x16,
    0x74, 0xCD, 0x26, 0x99, 0x0B, 0xEA,
    0x58, 0x13, 0xA8, 0x45, 0x02, 0xFB,
    0x67, 0x90, 0xDC, 0x3A, 0x1D, 0xB5,
    0x06, 0xE5, 0x7F, 0xCA, 0x20, 0x8E,
    0x39, 0xAD, 0x54, 0xF3, 0x11, 0x6C,
    0xCB, 0x79, 0x01, 0xA3, 0xDE, 0x35,
    0x24, 0xB0, 0x98, 0xE8, 0x5B, 0x0F,
    0xC8, 0x63, 0x1F, 0xDA, 0x76, 0x07,
    0x9F, 0xEE, 0x3C, 0x88, 0x14, 0xBF,
    0x22, 0x96, 0x5C, 0xD4, 0x6B, 0x1B,
    0xA7, 0x7A, 0x4C, 0xF7, 0x2D, 0x8C,
    0x7B, 0xA4, 0x39, 0xF1, 0x2D, 0xC8,
    0x10, 0xEE, 0x5A, 0x63, 0xB2, 0x08,
    0x47, 0x96, 0x3C, 0xE9, 0x14, 0x7E,
    0xA1, 0x6B, 0x2F, 0xD3, 0x50, 0xC7,
    0x1D, 0x9A, 0x84, 0xF8, 0x32, 0x0C,
    0xB6, 0x41, 0x75, 0xE3, 0x28, 0x99,
    0x0E, 0xD6, 0xAB, 0x3A, 0xFC, 0x60,
    0x12, 0xC2, 0x89, 0x5E, 0xF4, 0x07,
    0xAD, 0x34, 0xCB, 0x19, 0x78, 0xE2,
    0x95, 0x01, 0x6A, 0xDF, 0x3B, 0xB0,
    0x4C, 0xF2, 0x24, 0x87, 0x5D, 0xCE,
    0x0A, 0xA9, 0x38, 0xD5, 0x16, 0x6F,
    0xE5, 0x21, 0x9C, 0x43, 0xB9, 0x02,
    0xDA, 0x58, 0x73, 0xC0, 0x0F, 0xA6,
    0x35, 0xEB, 0x10, 0x92, 0x4F, 0xD1,
    0x7D, 0x08, 0xBC, 0x67, 0xFA, 0x2B,
    0x83, 0xC5, 0x1A, 0xE7, 0x90, 0x3D,
    0xB8, 0x44, 0x0B, 0xD8, 0x6C, 0xF0,
    0x27, 0xA3, 0x59, 0x0D, 0xCE, 0x36,
    0xE0, 0x7F, 0x13, 0xAC, 0x48, 0xD4,
    0x5F, 0x99, 0x20
};

static const uint8_t PAYLOAD_LED_WHITE[] = { PAYLOAD_TYPE_LED_WHITE };
static const uint8_t PAYLOAD_LED_RED[]   = { PAYLOAD_TYPE_LED_RED };
static const uint8_t PAYLOAD_LED_GREEN[] = { PAYLOAD_TYPE_LED_GREEN };

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

#define RX_WUR

//#define TX_WUR

context_t sx_wur_param;
context_t *sx_parameters = &sx_wur_param;



static void send_payload(payload_type_t type)
{
    const uint8_t *payload = NULL;
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

    ESP_LOGI(TAG, "Sending payload type %d with %d bytes", type, (int)length);
    Sx1261_InitParam_WUR(sx_parameters, SYNCWORD);
    Sx1261_InitFonction_WUR(sx_parameters);
    Sx1261_Send_WuR_Signal(sx_parameters, payload, length);
}

static void receive_varlen_payload(void)
{
    sx126x_rx_buffer_status_t rx_status;
    sx126x_get_rx_buffer_status(sx_parameters, &rx_status);

    uint8_t received_payload[256] = {0};
    sx126x_read_buffer(sx_parameters, rx_status.buffer_start_pointer,
                       received_payload, rx_status.pld_len_in_bytes);

    ESP_LOGI(TAG, "Received payload (%d bytes):", rx_status.pld_len_in_bytes);
    for (int i = 0; i < rx_status.pld_len_in_bytes; i++) {
        ESP_LOGI(TAG, "Byte[%d] = 0x%02X", i, received_payload[i]);
    }

    payload_type_t received_type = (payload_type_t)received_payload[0];

    if (received_type == PAYLOAD_TYPE_LED_WHITE) {
        ESP_LOGI(TAG, "Receive payload, LED ON");
        s_led_state = 1;
        blink_led();
        vTaskDelay(pdMS_TO_TICKS(500));
        s_led_state = 0;
        blink_led();
    }

    uint8_t color_code = 0;
    switch (received_type) {
        case PAYLOAD_TYPE_LED_WHITE:
            color_code = 1;
            break;
        case PAYLOAD_TYPE_LED_RED:
            color_code = 2;
            break;
        case PAYLOAD_TYPE_LED_GREEN:
            color_code = 3;
            break;
        default:
            break;
    }
    
    show_color_led(color_code);
    vTaskDelay(pdMS_TO_TICKS(500));
    show_color_led(0);

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
        /*Sx1261_InitParam_WUR(sx_parameters, SYNCWORD);
        Sx1261_InitFonction_WUR(sx_parameters);
        ESP_LOGI(TAG, "Sx1261_SendSignal_WUR");
        //Sx1261_Send_WuR_Signal(sx_parameters,data, 0); 
        Sx1261_Send_WuR_Signal(sx_parameters, payload, 1); 
        */


        //send_payload();
        send_payload(PAYLOAD_TYPE_ALLINFO);

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

            s_led_state = !s_led_state;
            blink_led();
            vTaskDelay(pdMS_TO_TICKS(1000));
            s_led_state = !s_led_state;
            blink_led();
			
        	vTaskDelay(pdMS_TO_TICKS(1000));
			//on repasse en sommeil

        
        
        receive_varlen_payload();

        /*
        if (received_payload[0] == 0x01) {          //have to put in the fuction receive_varlen_payload if needed
        ESP_LOGI(TAG, "Receive payload, LED ON");
        s_led_state = 1;
        blink_led();
        vTaskDelay(pdMS_TO_TICKS(500));
        s_led_state = 0;
        blink_led();
        }

        uint8_t color_code = 0;
        if (received_payload[0] == 0x01) {
            color_code = 1; // WHITE
        } else if (received_payload[0] == 0x02) {
            color_code = 2; // RED
        } else if (received_payload[0] == 0x03) {
            color_code = 3; // GREEN
        }

        show_color_led(color_code);
        vTaskDelay(pdMS_TO_TICKS(500));
        show_color_led(0); //clean the led
        */


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
    configure_led();
    s_led_state = !s_led_state;
    blink_led();
    vTaskDelay(pdMS_TO_TICKS(1000));
    s_led_state = !s_led_state;
    blink_led();

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
        receive_varlen_payload();

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
        vTaskDelay(pdMS_TO_TICKS(1000));

        //xTaskCreate(&run_RX_WUR, "run_RX_WUR", 1024*4, NULL, 5, NULL);
        
        uint64_t ext_wakeup_pins_mask = (1ULL << CONFIG_DIO1_GPIO);
        
        
        rtc_gpio_set_level(CONFIG_NSS_GPIO,1);
        rtc_gpio_pulldown_en(CONFIG_DIO1_GPIO);
        rtc_gpio_hold_en(CONFIG_NSS_GPIO);
        vTaskDelay(pdMS_TO_TICKS(1000));
    
        esp_sleep_enable_ext1_wakeup(ext_wakeup_pins_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
        ESP_LOGW(TAG, "GOING TO DEEP SLEEP");
        esp_deep_sleep_start();
        


    #endif

}






/* --- EOF ------------------------------------------------------------------ */