#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_heap_caps.h>

// Test framework includes
#include "test_manager.hpp"
#include "psram_test.hpp"
#include "bno055_test.hpp"
// Temporarily disabled for build compatibility
// #include "wifi_test.hpp"
// #include "ros2_test.hpp"

static const char *TAG = "TestMain";

// M5atomS3R GPIO definitions
#define LED_GPIO    GPIO_NUM_35      // RGB LED (WS2812)
#define BUTTON_GPIO GPIO_NUM_41      // Button
#define SDA_GPIO    GPIO_NUM_2       // I2C SDA
#define SCL_GPIO    GPIO_NUM_1       // I2C SCL

// Function declarations
extern "C" {
    void init_gpio(void);
    void test_execution_task(void *pvParameters);
    void system_monitor_task(void *pvParameters);
    void button_monitor_task(void *pvParameters);
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "=== M5atomS3R Class-Based Hardware Test Framework ===");
    ESP_LOGI(TAG, "ESP-IDF Version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Chip Model: %s", CONFIG_IDF_TARGET);
    ESP_LOGI(TAG, "Free Heap: %" PRIu32 " bytes", esp_get_free_heap_size());
    
    // Display system information
    ESP_LOGI(TAG, "=== System Information ===");
    ESP_LOGI(TAG, "PSRAM total: %d bytes", heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI(TAG, "PSRAM free: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI(TAG, "Internal RAM total: %d bytes", heap_caps_get_total_size(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, "Internal RAM free: %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize GPIO
    init_gpio();
    
    // Create tasks
    xTaskCreate(&test_execution_task, "test_execution_task", 8192, NULL, 5, NULL);
    xTaskCreate(&system_monitor_task, "system_monitor_task", 4096, NULL, 3, NULL);
    xTaskCreate(&button_monitor_task, "button_monitor_task", 2048, NULL, 4, NULL);
    
    ESP_LOGI(TAG, "Class-based test framework initialized successfully!");
    ESP_LOGI(TAG, "Test execution will begin shortly...");
}

void init_gpio(void)
{
    // Configure button GPIO
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&button_config);
    
    ESP_LOGI(TAG, "GPIO initialized");
}

void test_execution_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Test execution task started");
    
    // Wait for system to stabilize
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    ESP_LOGI(TAG, "Creating test manager and test instances");
    
    // Create test manager
    TestManager test_manager;
    
    // Configure test manager
    test_manager.setStopOnFirstFailure(false);  // Continue even if tests fail
    test_manager.setTestTimeout(300000);        // 5 minutes per test
    
    // Create and configure PSRAM test
    auto psram_test = std::make_unique<PSRAMTest>();
    psram_test->setMinExpectedSize(8 * 1024 * 1024);  // 8MB
    psram_test->setAllocationTestSize(1024 * 1024);   // 1MB test allocation
    test_manager.addTest(std::unique_ptr<BaseTest>(std::move(psram_test)));
    
    // Create and configure BNO055 test
    auto bno055_test = std::make_unique<BNO055Test>();
    bno055_test->setI2CConfig(I2C_NUM_0, SDA_GPIO, SCL_GPIO, 100000);
    bno055_test->setReadingCount(10);
    bno055_test->setStabilityTestDuration(10000);  // 10 seconds
    bno055_test->setQuaternionTolerance(0.1f);
    test_manager.addTest(std::unique_ptr<BaseTest>(std::move(bno055_test)));
    
    // WiFi and ROS2 tests temporarily disabled for ESP-IDF library compatibility issues
    ESP_LOGI(TAG, "WiFi and ROS2 tests disabled due to ESP-IDF library compatibility");
    ESP_LOGI(TAG, "Running PSRAM and BNO055 tests only");
    
    // TODO: Re-enable after ESP-IDF submodule update:
    // - WiFi connection test 
    // - ROS2 communication test
    
    ESP_LOGI(TAG, "Added %zu tests to test manager", test_manager.getTestCount());
    
    // Display test information
    auto test_names = test_manager.getTestNames();
    ESP_LOGI(TAG, "Registered tests:");
    for (const auto& name : test_names) {
        ESP_LOGI(TAG, "  - %s", name.c_str());
    }
    
    ESP_LOGI(TAG, "Starting test execution...");
    
    // Run all tests
    test_manager.runAllTests();
    
    // Display results
    test_manager.printTestResults();
    
    // Log final result
    TestResult overall_result = test_manager.getOverallResult();
    if (overall_result == TestResult::PASSED) {
        ESP_LOGI(TAG, "🎉 ALL TESTS PASSED! 🎉");
    } else {
        ESP_LOGE(TAG, "❌ SOME TESTS FAILED ❌");
    }
    
    // Get and display statistics
    auto stats = test_manager.getStatistics();
    ESP_LOGI(TAG, "Final Statistics:");
    ESP_LOGI(TAG, "  Success Rate: %.1f%% (%lu/%lu tests passed)", 
             stats.success_rate, stats.passed_tests, stats.total_tests);
    ESP_LOGI(TAG, "  Total Execution Time: %.1f seconds", 
             stats.total_duration_ms / 1000.0f);
    
    ESP_LOGI(TAG, "Test execution completed. Task will continue monitoring...");
    
    // Continue running to allow monitoring
    while (1) {
        // Log periodic status
        ESP_LOGI(TAG, "Test framework idle. Overall result: %s", 
                 BaseTest::resultToString(overall_result).c_str());
        
        vTaskDelay(pdMS_TO_TICKS(60000));  // Log every minute
    }
}

void system_monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "System monitor task started");
    
    int counter = 0;
    
    while (1) {
        counter++;
        
        // Every 30 seconds, log system information
        if (counter % 6 == 0) {
            ESP_LOGI(TAG, "=== System Status (cycle %d) ===", counter / 6);
            ESP_LOGI(TAG, "Free Heap: %" PRIu32 " bytes", esp_get_free_heap_size());
            ESP_LOGI(TAG, "PSRAM free: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
            ESP_LOGI(TAG, "Internal RAM free: %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
            ESP_LOGI(TAG, "Uptime: %d seconds", counter * 5);
        } else {
            // Brief status
            ESP_LOGI(TAG, "System monitor: Free heap %" PRIu32 " bytes (cycle %d)", 
                     esp_get_free_heap_size(), counter);
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // 5 seconds
    }
}

void button_monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Button monitor task started");
    
    int last_button_state = 1;
    int button_press_count = 0;
    
    while (1) {
        int current_button_state = gpio_get_level(BUTTON_GPIO);
        
        if (last_button_state != current_button_state) {
            if (current_button_state == 0) {
                button_press_count++;
                ESP_LOGI(TAG, "🔘 Button pressed! (count: %d)", button_press_count);
                
                // Could trigger specific actions based on button presses
                if (button_press_count % 5 == 0) {
                    ESP_LOGI(TAG, "Button pressed %d times - system is responsive!", button_press_count);
                }
            } else {
                ESP_LOGI(TAG, "Button released!");
            }
            last_button_state = current_button_state;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  // 50ms debounce
    }
}