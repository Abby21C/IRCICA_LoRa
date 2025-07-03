idf_component_register(
    SRCS "src/wur_transmission.c"
    INCLUDE_DIRS "inc"
    REQUIRES payload_manager led_status ra01s esp_timer
)
