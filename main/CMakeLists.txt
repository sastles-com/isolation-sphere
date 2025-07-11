idf_component_register(SRCS "test_main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES test_framework hardware_test nvs_flash driver bno055 wifi_manager ros2_manager esp_system esp_wifi freertos)