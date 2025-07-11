#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_heap_caps.h>
#include "hardware_info.hpp"
#include "bno055.h"
#include "wifi_manager.h"

static const char *TAG = "M5ATOMS3R";

// M5atomS3R GPIO definitions
#define LED_GPIO    GPIO_NUM_35      // RGB LED (WS2812)
#define BUTTON_GPIO GPIO_NUM_41      // Button
#define SDA_GPIO    GPIO_NUM_2       // I2C SDA
#define SCL_GPIO    GPIO_NUM_1       // I2C SCL

// Function declarations
extern "C" {
    void init_gpio(void);
    void hello_world_task(void *pvParameters);
    void button_task(void *pvParameters);
    void hardware_test_task(void *pvParameters);
    void bno055_test_task(void *pvParameters);
    void wifi_test_task(void *pvParameters);
    void wifi_event_callback(wifi_status_t status, wifi_info_t *info);
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "=== M5atomS3R Hardware Test ===");
    ESP_LOGI(TAG, "ESP-IDF Version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Chip Model: %s", CONFIG_IDF_TARGET);
    ESP_LOGI(TAG, "Free Heap: %" PRIu32 " bytes", esp_get_free_heap_size());
    
    // PSRAM詳細情報
    ESP_LOGI(TAG, "=== PSRAM Information ===");
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
    xTaskCreate(&hardware_test_task, "hardware_test_task", 4096, NULL, 5, NULL);
    xTaskCreate(&hello_world_task, "hello_world_task", 2048, NULL, 4, NULL);
    xTaskCreate(&button_task, "button_task", 2048, NULL, 4, NULL);
    xTaskCreate(&bno055_test_task, "bno055_test_task", 4096, NULL, 3, NULL);
    xTaskCreate(&wifi_test_task, "wifi_test_task", 4096, NULL, 2, NULL);
    
    ESP_LOGI(TAG, "M5atomS3R Hardware Test initialized successfully!");
    ESP_LOGI(TAG, "Press the button to test GPIO functionality");
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

void hardware_test_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Hardware test task started");
    
    // ハードウェア情報クラスのインスタンス作成
    HardwareInfo hw_info;
    
    if (!hw_info.isInitialized()) {
        ESP_LOGE(TAG, "Failed to initialize hardware info");
        vTaskDelete(NULL);
        return;
    }
    
    // 初回の詳細ハードウェア情報出力
    std::string hw_info_str = hw_info.getAllInfoAsString();
    ESP_LOGI(TAG, "\n%s", hw_info_str.c_str());
    
    int counter = 0;
    while (1) {
        counter++;
        
        // 5回ごとに詳細情報を再出力
        if (counter % 5 == 0) {
            hw_info_str = hw_info.getAllInfoAsString();
            ESP_LOGI(TAG, "\n%s", hw_info_str.c_str());
        } else {
            // 簡易情報のみ出力
            HardwareInfo::MemoryInfo mem_info;
            if (hw_info.getMemoryInfo(mem_info)) {
                ESP_LOGI(TAG, "Free Heap: %" PRIu32 " bytes, Test Cycle: %d", 
                        mem_info.free_heap_bytes, counter);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000)); // 10 seconds
    }
}

void hello_world_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Hello World task started");
    
    int counter = 0;
    
    while (1) {
        counter++;
        ESP_LOGI(TAG, "Hello World from M5atomS3R! Counter: %d", counter);
        ESP_LOGI(TAG, "Free heap: %" PRIu32 " bytes", esp_get_free_heap_size());
        ESP_LOGI(TAG, "Up time: %d seconds", counter * 2);
        
        // PSRAM情報の追加
        ESP_LOGI(TAG, "PSRAM free: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        ESP_LOGI(TAG, "PSRAM total: %d bytes", heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
        ESP_LOGI(TAG, "Internal RAM free: %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 seconds
    }
}

void button_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Button task started");
    
    int last_button_state = 1;
    
    while (1) {
        int current_button_state = gpio_get_level(BUTTON_GPIO);
        
        if (last_button_state != current_button_state) {
            if (current_button_state == 0) {
                ESP_LOGI(TAG, "Button pressed!");
            } else {
                ESP_LOGI(TAG, "Button released!");
            }
            last_button_state = current_button_state;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50)); // 50ms debounce
    }
}

void bno055_test_task(void *pvParameters)
{
    ESP_LOGI(TAG, "BNO055 test task started");
    
    // BNO055 configuration
    bno055_config_t config = {
        .i2c_port = I2C_NUM_0,
        .sda_pin = SDA_GPIO,
        .scl_pin = SCL_GPIO,
        .i2c_freq = 100000,  // 100kHz
        .i2c_addr = BNO055_I2C_ADDR
    };
    
    // Initialize BNO055
    esp_err_t ret = bno055_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BNO055 initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check I2C connections and sensor power");
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "BNO055 initialized successfully");
    
    // Wait for sensor calibration
    ESP_LOGI(TAG, "Waiting for sensor stabilization...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    int counter = 0;
    while (1) {
        counter++;
        
        bno055_quaternion_t quat;
        ret = bno055_get_quaternion(&quat);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "=== BNO055 Quaternion Data #%d ===", counter);
            ESP_LOGI(TAG, "W: %+.4f", quat.w);
            ESP_LOGI(TAG, "X: %+.4f", quat.x);
            ESP_LOGI(TAG, "Y: %+.4f", quat.y);
            ESP_LOGI(TAG, "Z: %+.4f", quat.z);
            
            // Calculate magnitude for verification (should be close to 1.0)
            float magnitude = sqrtf(quat.w*quat.w + quat.x*quat.x + quat.y*quat.y + quat.z*quat.z);
            ESP_LOGI(TAG, "Magnitude: %.4f", magnitude);
            ESP_LOGI(TAG, "==============================");
        } else {
            ESP_LOGE(TAG, "Failed to read quaternion: %s", esp_err_to_name(ret));
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second interval
    }
}

void wifi_event_callback(wifi_status_t status, wifi_info_t *info)
{
    switch (status) {
        case WIFI_STATUS_CONNECTING:
            ESP_LOGI(TAG, "WiFi: Connecting... (retry: %d)", info->retry_count);
            break;
            
        case WIFI_STATUS_CONNECTED:
            ESP_LOGI(TAG, "=== WiFi Connected Successfully ===");
            ESP_LOGI(TAG, "SSID: %s", info->ssid);
            ESP_LOGI(TAG, "IP Address: %s", info->ip_addr);
            ESP_LOGI(TAG, "Gateway: %s", info->gateway);
            ESP_LOGI(TAG, "Netmask: %s", info->netmask);
            ESP_LOGI(TAG, "RSSI: %d dBm", info->rssi);
            ESP_LOGI(TAG, "Channel: %d", info->channel);
            ESP_LOGI(TAG, "Connection time: %lu ms", info->connection_time_ms);
            ESP_LOGI(TAG, "==================================");
            break;
            
        case WIFI_STATUS_DISCONNECTED:
            ESP_LOGI(TAG, "WiFi: Disconnected");
            break;
            
        case WIFI_STATUS_FAILED:
            ESP_LOGE(TAG, "WiFi: Connection failed after %d retries", info->retry_count);
            break;
            
        case WIFI_STATUS_TIMEOUT:
            ESP_LOGE(TAG, "WiFi: Connection timeout");
            break;
            
        default:
            ESP_LOGW(TAG, "WiFi: Unknown status %d", status);
            break;
    }
}

void wifi_test_task(void *pvParameters)
{
    ESP_LOGI(TAG, "WiFi test task started");
    
    // Wait for other systems to initialize
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Initialize WiFi manager
    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi manager initialization failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "WiFi manager initialized successfully");
    
    // Set event callback
    wifi_manager_set_callback(wifi_event_callback);
    
    // Configure target WiFi network
    wifi_manager_config_t config = {
        .ssid = "ros2_atom_ap",
        .password = "isolation-sphere",
        .max_retry = 5,
        .timeout_ms = 15000,  // 15 seconds timeout
        .auto_reconnect = true
    };
    
    ESP_LOGI(TAG, "=== WiFi Connection Test ===");
    ESP_LOGI(TAG, "Target SSID: %s", config.ssid);
    ESP_LOGI(TAG, "Max retries: %d", config.max_retry);
    ESP_LOGI(TAG, "Timeout: %lu ms", config.timeout_ms);
    ESP_LOGI(TAG, "===========================");
    
    // Attempt connection
    ESP_LOGI(TAG, "Attempting to connect to WiFi...");
    ret = wifi_manager_connect(&config);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "WiFi connection successful!");
        
        // Monitor connection status
        while (1) {
            if (wifi_manager_is_connected()) {
                wifi_info_t info;
                if (wifi_manager_get_info(&info) == ESP_OK) {
                    ESP_LOGI(TAG, "WiFi Status: Connected to %s (RSSI: %d dBm, IP: %s)", 
                             info.ssid, info.rssi, info.ip_addr);
                }
            } else {
                ESP_LOGW(TAG, "WiFi Status: Disconnected");
                
                // Attempt reconnection
                ESP_LOGI(TAG, "Attempting to reconnect...");
                ret = wifi_manager_connect(&config);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Reconnection failed: %s", esp_err_to_name(ret));
                }
            }
            
            vTaskDelay(pdMS_TO_TICKS(10000)); // Check every 10 seconds
        }
        
    } else {
        ESP_LOGE(TAG, "WiFi connection failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Please check:");
        ESP_LOGE(TAG, "1. WiFi SSID 'ros2_atom_ap' is available");
        ESP_LOGE(TAG, "2. Password 'isolation-sphere' is correct");
        ESP_LOGE(TAG, "3. WiFi signal strength is sufficient");
    }
    
    // Task ends here if connection fails
    vTaskDelete(NULL);
}