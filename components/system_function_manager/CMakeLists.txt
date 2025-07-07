idf_component_register(
    SRCS 
        "system_setup.c"
        "firmware_update.c"
        "ota_server.c"
        "wur_handler.c"
    INCLUDE_DIRS "."

    REQUIRES
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        esp_http_client
        led_status
        ota_update
        http_ota_server
        async_web_server
        payload_manager
        wur_transmission
        utils
)
