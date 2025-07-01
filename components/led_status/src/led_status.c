#include "led_status.h"
#include "led_strip.h"
#include "esp_log.h"

static const char* TAG = "led_status";

static led_strip_handle_t led_strip = NULL;
static bool led_on = false;

void led_status_init(uint8_t gpio_num)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio_num,
        .max_leds = 1,
    };

    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI3_HOST,
        .flags.with_dma = true,
    };

    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
    led_strip_clear(led_strip);
}

void led_status_set_state(bool on)
{
    led_on = on;
    if (led_on) {
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);
    } else {
        led_strip_clear(led_strip);
    }
    led_strip_refresh(led_strip);
}

void led_status_toggle(void)
{
    led_status_set_state(!led_on);
}

void led_status_show_color(led_color_t color)
{
    switch (color) {
        case LED_COLOR_WHITE:
            led_strip_set_pixel(led_strip, 0, 16, 16, 16); break;
        case LED_COLOR_RED:
            led_strip_set_pixel(led_strip, 0, 48, 0, 0); break;
        case LED_COLOR_GREEN:
            led_strip_set_pixel(led_strip, 0, 0, 48, 0); break;
        default:
            led_strip_clear(led_strip); break;
    }
    led_strip_refresh(led_strip);
}

void led_status_handle_payload(uint8_t payload_type)
{
    switch (payload_type) {
        case 1:
            led_status_set_state(true);
            vTaskDelay(pdMS_TO_TICKS(500));
            led_status_set_state(false);
            break;
        case 2:
            led_status_show_color(LED_COLOR_RED);
            vTaskDelay(pdMS_TO_TICKS(500));
            led_status_show_color(LED_COLOR_NONE);
            break;
        case 3:
            led_status_show_color(LED_COLOR_GREEN);
            vTaskDelay(pdMS_TO_TICKS(500));
            led_status_show_color(LED_COLOR_NONE);
            break;
        default:
            break;
    }
}
