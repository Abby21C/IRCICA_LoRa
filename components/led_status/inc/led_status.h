#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    LED_COLOR_NONE = 0,
    LED_COLOR_WHITE,
    LED_COLOR_RED,
    LED_COLOR_GREEN
} led_color_t;

void led_status_init(uint8_t gpio_num);
void led_status_set_state(bool on);
void led_status_toggle(void);
void led_status_show_color(led_color_t color);
void led_status_handle_payload(uint8_t payload_type);
