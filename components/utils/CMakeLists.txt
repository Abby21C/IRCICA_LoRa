# components/utils/CMakeLists.txt
idf_component_register(SRCS "src/utils.c"
                    INCLUDE_DIRS "inc"
                    REQUIRES ra01s esp_netif)
