
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

/**
 * @brief simple task that send a WuB each 3 secondes
 * 
 */
void run_TX_WUR(void *pvParameters)
{
    uint64_t exec_time = 0;
    uint8_t compteur = 0;
    uint8_t data[256] = {0};
    while (1)
    {
        Sx1261_InitParam_WUR(sx_parameters, SYNCWORD);
        Sx1261_InitFonction_WUR(sx_parameters);
        ESP_LOGI(TAG, "Sx1261_SendSignal_WUR");
        Sx1261_Send_WuR_Signal(sx_parameters,data, 0);
        compteur++;
        ESP_LOGI(TAG, "number of signal sended: %d", compteur);
        vTaskDelay(pdMS_TO_TICKS(30000));
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
        uint64_t ext_wakeup_pins_mask = (1ULL << CONFIG_DIO1_GPIO);
        
        rtc_gpio_set_direction_in_sleep(CONFIG_DIO1_GPIO,RTC_GPIO_MODE_INPUT_ONLY);
        rtc_gpio_set_direction_in_sleep(CONFIG_BUSY_GPIO,RTC_GPIO_MODE_INPUT_ONLY);
        rtc_gpio_set_direction_in_sleep(CONFIG_NSS_GPIO,RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_level(CONFIG_NSS_GPIO,1);
        rtc_gpio_pulldown_en(CONFIG_DIO1_GPIO);
        rtc_gpio_pullup_dis(CONFIG_BUSY_GPIO);
        rtc_gpio_pulldown_en(CONFIG_BUSY_GPIO);
        rtc_gpio_pulldown_dis(CONFIG_NSS_GPIO);
        rtc_gpio_pullup_en(CONFIG_NSS_GPIO);
        rtc_gpio_hold_en(CONFIG_NSS_GPIO);
        vTaskDelay(pdMS_TO_TICKS(1000));
    
        esp_sleep_enable_ext1_wakeup(ext_wakeup_pins_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
        esp_deep_sleep_start();


    #endif

}






/* --- EOF ------------------------------------------------------------------ */