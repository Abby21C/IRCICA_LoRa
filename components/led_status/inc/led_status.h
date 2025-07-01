#ifndef LED_STATUS_H
#define LED_STATUS_H

#include <stdint.h>

typedef enum {
    LED_COLOR_OFF = 0,
    LED_COLOR_WHITE,
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    // More colors can be extended
} led_color_t;

void led_status_init(void);              // Initialize LED
void led_set_color(led_color_t color);   // Set LED color
void led_blink_once(void);               // Blink once (optional)
void led_blink_pattern(const led_color_t *pattern, int len);  // Pattern blink (optional)

#endif
