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

static const char *TAG = "M5ATOMS3R";

// M5atomS3R GPIO definitions
#define LED_GPIO    GPIO_NUM_35      // RGB LED (WS2812)
#define BUTTON_GPIO GPIO_NUM_41      // Button
// #define SDA_GPIO    GPIO_NUM_38     // I2C SDA
// #define SCL_GPIO    GPIO_NUM_39      // I2C SCL
#define SDA_GPIO    GPIO_NUM_2     // I2C SDA
#define SCL_GPIO    GPIO_NUM_1      // I2C SCL

// Function declarations
void init_gpio(void);
void hello_world_task(void *pvParameters);
void button_task(void *pvParameters);

void app_main(void)
{
    ESP_LOGI(TAG, "=== M5atomS3R Hello World ===");
    ESP_LOGI(TAG, "ESP-IDF Version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Chip Model: %s", CONFIG_IDF_TARGET);
    ESP_LOGI(TAG, "Free Heap: %"PRIu32" bytes", esp_get_free_heap_size());
    
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
    xTaskCreate(&hello_world_task, "hello_world_task", 2048, NULL, 5, NULL);
    xTaskCreate(&button_task, "button_task", 2048, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "M5atomS3R Hello World initialized successfully!");
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

void hello_world_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Hello World task started");
    
    int counter = 0;
    
    while (1) {
        counter++;
        ESP_LOGI(TAG, "Hello World from M5atomS3R! Counter: %d", counter);
        ESP_LOGI(TAG, "Free heap: %"PRIu32" bytes", esp_get_free_heap_size());
        ESP_LOGI(TAG, "Up time: %d seconds", counter * 2);
        
        // Print separator for readability
        if (counter % 5 == 0) {
            ESP_LOGI(TAG, "--- System Status Report ---");
            ESP_LOGI(TAG, "Device: M5atomS3R (ESP32-S3)");
            ESP_LOGI(TAG, "Runtime: %d seconds", counter * 2);
            ESP_LOGI(TAG, "Status: Running OK");
            ESP_LOGI(TAG, "----------------------------");
        }
        
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