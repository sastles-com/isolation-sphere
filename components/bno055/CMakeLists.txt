idf_component_register(
    SRCS "src/bno055.c"
    INCLUDE_DIRS "include"
    REQUIRES driver freertos esp_common
)