idf_component_register(
    SRCS "src/http_ota_server.c"
    INCLUDE_DIRS "inc"
    REQUIRES esp_http_server esp_http_client esp_https_ota esp_wifi nvs_flash app_update async_web_server
)
