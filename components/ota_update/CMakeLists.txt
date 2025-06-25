idf_component_register(SRCS "src/ota_update.c"
                      INCLUDE_DIRS "inc"
                      REQUIRES esp_http_client esp_https_ota app_update)

