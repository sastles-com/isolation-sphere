#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/ip4_addr.h"
#include <string.h>

static const char *TAG = "WIFI_MANAGER";

// WiFi event bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Global variables
static bool wifi_manager_initialized = false;
static wifi_status_t current_status = WIFI_STATUS_DISCONNECTED;
static wifi_info_t current_info = {0};
static wifi_manager_config_t current_config = {0};
static wifi_event_callback_t event_callback = NULL;
static EventGroupHandle_t wifi_event_group = NULL;
static uint8_t retry_count = 0;
static uint32_t connection_start_time = 0;

// WiFi scan results
static wifi_ap_record_t *scan_results = NULL;
static uint16_t scan_count = 0;

// Forward declarations
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void update_wifi_info(void);
static void notify_status_change(wifi_status_t new_status);

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi station started");
        esp_wifi_connect();
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGI(TAG, "WiFi disconnected, reason: %d", disconnected->reason);
        
        if (retry_count < current_config.max_retry) {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGI(TAG, "Retry connecting to WiFi (%d/%d)", retry_count, current_config.max_retry);
            notify_status_change(WIFI_STATUS_CONNECTING);
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Failed to connect to WiFi after %d retries", current_config.max_retry);
            notify_status_change(WIFI_STATUS_FAILED);
        }
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        
        retry_count = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        notify_status_change(WIFI_STATUS_CONNECTED);
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "WiFi scan completed");
        
        // Get scan results
        esp_wifi_scan_get_ap_num(&scan_count);
        if (scan_count > 0) {
            if (scan_results) {
                free(scan_results);
            }
            scan_results = malloc(sizeof(wifi_ap_record_t) * scan_count);
            if (scan_results) {
                esp_wifi_scan_get_ap_records(&scan_count, scan_results);
                ESP_LOGI(TAG, "Found %d WiFi networks", scan_count);
            }
        }
    }
}

static void update_wifi_info(void)
{
    memset(&current_info, 0, sizeof(wifi_info_t));
    current_info.status = current_status;
    current_info.retry_count = retry_count;
    
    if (current_status == WIFI_STATUS_CONNECTED) {
        // Get WiFi info
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            strncpy(current_info.ssid, (char*)ap_info.ssid, sizeof(current_info.ssid) - 1);
            current_info.rssi = ap_info.rssi;
            current_info.channel = ap_info.primary;
        }
        
        // Get IP info
        esp_netif_ip_info_t ip_info;
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            snprintf(current_info.ip_addr, sizeof(current_info.ip_addr), 
                    IPSTR, IP2STR(&ip_info.ip));
            snprintf(current_info.gateway, sizeof(current_info.gateway), 
                    IPSTR, IP2STR(&ip_info.gw));
            snprintf(current_info.netmask, sizeof(current_info.netmask), 
                    IPSTR, IP2STR(&ip_info.netmask));
        }
        
        current_info.connection_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS - connection_start_time;
    }
}

static void notify_status_change(wifi_status_t new_status)
{
    current_status = new_status;
    update_wifi_info();
    
    if (event_callback) {
        event_callback(new_status, &current_info);
    }
}

esp_err_t wifi_manager_init(void)
{
    if (wifi_manager_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing WiFi manager...");
    
    // Initialize TCP/IP
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TCP/IP: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Create default event loop
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Create default WiFi station
    esp_netif_create_default_wifi_sta();
    
    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Create event group
    wifi_event_group = xEventGroupCreate();
    if (!wifi_event_group) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }
    
    // Register event handlers
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Set WiFi mode
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(ret));
        return ret;
    }
    
    wifi_manager_initialized = true;
    current_status = WIFI_STATUS_DISCONNECTED;
    
    ESP_LOGI(TAG, "WiFi manager initialized successfully");
    return ESP_OK;
}

esp_err_t wifi_manager_deinit(void)
{
    if (!wifi_manager_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deinitializing WiFi manager...");
    
    // Stop WiFi
    esp_wifi_stop();
    esp_wifi_deinit();
    
    // Unregister event handlers
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler);
    
    // Delete event group
    if (wifi_event_group) {
        vEventGroupDelete(wifi_event_group);
        wifi_event_group = NULL;
    }
    
    // Free scan results
    if (scan_results) {
        free(scan_results);
        scan_results = NULL;
        scan_count = 0;
    }
    
    wifi_manager_initialized = false;
    current_status = WIFI_STATUS_DISCONNECTED;
    
    ESP_LOGI(TAG, "WiFi manager deinitialized");
    return ESP_OK;
}

esp_err_t wifi_manager_connect(const wifi_manager_config_t *config)
{
    if (!wifi_manager_initialized || !config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", config->ssid);
    
    // Store configuration
    memcpy(&current_config, config, sizeof(wifi_manager_config_t));
    retry_count = 0;
    connection_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Configure WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    
    strncpy((char*)wifi_config.sta.ssid, config->ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, config->password, sizeof(wifi_config.sta.password) - 1);
    
    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Start WiFi
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        return ret;
    }
    
    notify_status_change(WIFI_STATUS_CONNECTING);
    
    // Wait for connection
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                          pdFALSE,
                                          pdFALSE,
                                          pdMS_TO_TICKS(config->timeout_ms));
    
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to WiFi SSID: %s", config->ssid);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to WiFi SSID: %s", config->ssid);
        return ESP_ERR_WIFI_CONN;
    } else {
        ESP_LOGE(TAG, "Connection timeout for WiFi SSID: %s", config->ssid);
        notify_status_change(WIFI_STATUS_TIMEOUT);
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t wifi_manager_disconnect(void)
{
    if (!wifi_manager_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Disconnecting from WiFi");
    
    esp_err_t ret = esp_wifi_disconnect();
    if (ret == ESP_OK) {
        notify_status_change(WIFI_STATUS_DISCONNECTED);
    }
    
    return ret;
}

wifi_status_t wifi_manager_get_status(void)
{
    return current_status;
}

esp_err_t wifi_manager_get_info(wifi_info_t *info)
{
    if (!info) {
        return ESP_ERR_INVALID_ARG;
    }
    
    update_wifi_info();
    memcpy(info, &current_info, sizeof(wifi_info_t));
    return ESP_OK;
}

bool wifi_manager_is_connected(void)
{
    return current_status == WIFI_STATUS_CONNECTED;
}

void wifi_manager_set_callback(wifi_event_callback_t callback)
{
    event_callback = callback;
}

esp_err_t wifi_manager_scan_start(void)
{
    if (!wifi_manager_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Starting WiFi scan");
    
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };
    
    return esp_wifi_scan_start(&scan_config, false);
}

uint16_t wifi_manager_get_scan_count(void)
{
    return scan_count;
}

esp_err_t wifi_manager_get_scan_result(uint16_t index, wifi_ap_record_t *ap_info)
{
    if (!ap_info || index >= scan_count || !scan_results) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(ap_info, &scan_results[index], sizeof(wifi_ap_record_t));
    return ESP_OK;
}