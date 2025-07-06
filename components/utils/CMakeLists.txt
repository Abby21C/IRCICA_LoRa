# components/utils/CMakeLists.txt
idf_component_register(SRCS "src/utils.c"
                    INCLUDE_DIRS "inc"
                    REQUIRES ra01s esp_netif)

target_include_directories(${COMPONENT_LIB} PRIVATE ${PROJECT_DIR}/config)
