idf_component_register(
    SRCS "src/async_web_server.c"
    INCLUDE_DIRS "inc"
    REQUIRES esp_http_server freertos
)
