idf_component_register(
    SRCS "src/hardware_info.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_system esp_hw_support spi_flash heap driver nvs_flash esp_common esp_psram
)