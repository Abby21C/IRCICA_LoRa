set(component_srcs 
"ra01s.c"
"sx126x.c"
"sx126x_lr_fhss.c"
"lr_fhss_mac.c"
"wur/src/sx1261_WuR.c")

idf_component_register(SRCS "${component_srcs}"
                       #PRIV_REQUIRES driver
                       REQUIRES driver
                       INCLUDE_DIRS "." "wur/inc")

target_include_directories(${COMPONENT_LIB} PRIVATE ${PROJECT_DIR}/config)
