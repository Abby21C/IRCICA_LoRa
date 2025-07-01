#include "led_status.h"
#include "led_strip.h"
#include "esp_log.h"

#define LED_GPIO 48

static const char *TAG = "LED_STATUS";
static led_strip_handle_t led_strip = NULL;

void led_status_init(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = 1,
    };

    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI3_HOST,
        .flags.with_dma = true,
    };

    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
    led_strip_clear(led_strip);
}

void led_set_color(led_color_t color)
{
    switch (color) {
        case LED_COLOR_WHITE:
            led_strip_set_pixel(led_strip, 0, 16, 16, 16);
            break;
        case LED_COLOR_RED:
            led_strip_set_pixel(led_strip, 0, 48, 0, 0);
            break;
        case LED_COLOR_GREEN:
            led_strip_set_pixel(led_strip, 0, 0, 48, 0);
            break;
        case LED_COLOR_OFF:
        default:
            led_strip_clear(led_strip);
            return;
    }
    led_strip_refresh(led_strip);
}
