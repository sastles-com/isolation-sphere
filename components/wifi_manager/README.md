# WiFi Manager Component

ESP-IDF component for WiFi station mode management with event handling and connection monitoring.

## Overview

The WiFi Manager provides a high-level interface for WiFi connectivity on ESP32-S3, featuring automatic connection management, status monitoring, and event-driven callbacks.

## Features

- WiFi station mode initialization and management
- Automatic connection and reconnection handling
- Real-time connection status monitoring
- Event-driven callback system
- WiFi network scanning capabilities
- Comprehensive error handling and timeout management
- IP address and network information retrieval

## Usage

### 1. Include the header

```c
#include "wifi_manager.h"
```

### 2. Initialize the WiFi manager

```c
esp_err_t ret = wifi_manager_init();
if (ret != ESP_OK) {
    ESP_LOGE("MAIN", "WiFi manager initialization failed");
}
```

### 3. Set up event callback (optional)

```c
void wifi_event_callback(wifi_status_t status, wifi_info_t *info) {
    switch (status) {
        case WIFI_STATUS_CONNECTED:
            ESP_LOGI("MAIN", "Connected to %s, IP: %s", info->ssid, info->ip_addr);
            break;
        case WIFI_STATUS_DISCONNECTED:
            ESP_LOGI("MAIN", "WiFi disconnected");
            break;
        // Handle other statuses...
    }
}

wifi_manager_set_callback(wifi_event_callback);
```

### 4. Connect to WiFi network

```c
wifi_config_t config = {
    .ssid = "ros2_atom_ap",
    .password = "isolation-sphere",
    .max_retry = 5,
    .timeout_ms = 15000,
    .auto_reconnect = true
};

esp_err_t ret = wifi_manager_connect(&config);
if (ret == ESP_OK) {
    ESP_LOGI("MAIN", "WiFi connection successful");
}
```

### 5. Monitor connection status

```c
if (wifi_manager_is_connected()) {
    wifi_info_t info;
    wifi_manager_get_info(&info);
    ESP_LOGI("MAIN", "Connected to %s, IP: %s, RSSI: %d dBm", 
             info.ssid, info.ip_addr, info.rssi);
}
```

### 6. Cleanup

```c
wifi_manager_deinit();
```

## API Reference

### Configuration Structures

#### wifi_config_t
```c
typedef struct {
    char ssid[32];          // WiFi SSID
    char password[64];      // WiFi password
    uint8_t max_retry;      // Maximum retry attempts
    uint32_t timeout_ms;    // Connection timeout in milliseconds
    bool auto_reconnect;    // Enable automatic reconnection
} wifi_config_t;
```

#### wifi_info_t
```c
typedef struct {
    wifi_status_t status;   // Current connection status
    char ssid[32];          // Connected SSID
    uint8_t rssi;           // Signal strength (dBm)
    uint8_t channel;        // WiFi channel
    char ip_addr[16];       // IP address
    char gateway[16];       // Gateway address
    char netmask[16];       // Network mask
    uint32_t connection_time_ms; // Connection duration
    uint8_t retry_count;    // Current retry count
} wifi_info_t;
```

### Connection Status

```c
typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,  // Not connected
    WIFI_STATUS_CONNECTING,        // Connection in progress
    WIFI_STATUS_CONNECTED,         // Successfully connected
    WIFI_STATUS_FAILED,            // Connection failed
    WIFI_STATUS_TIMEOUT            // Connection timeout
} wifi_status_t;
```

### Core Functions

#### Initialization
- `esp_err_t wifi_manager_init(void)` - Initialize WiFi manager
- `esp_err_t wifi_manager_deinit(void)` - Deinitialize and cleanup

#### Connection Management
- `esp_err_t wifi_manager_connect(const wifi_config_t *config)` - Connect to WiFi
- `esp_err_t wifi_manager_disconnect(void)` - Disconnect from WiFi
- `bool wifi_manager_is_connected(void)` - Check connection status

#### Status and Information
- `wifi_status_t wifi_manager_get_status(void)` - Get current status
- `esp_err_t wifi_manager_get_info(wifi_info_t *info)` - Get detailed information

#### Event Handling
- `void wifi_manager_set_callback(wifi_event_callback_t callback)` - Set event callback

#### Network Scanning
- `esp_err_t wifi_manager_scan_start(void)` - Start WiFi scan
- `uint16_t wifi_manager_get_scan_count(void)` - Get scan results count
- `esp_err_t wifi_manager_get_scan_result(uint16_t index, wifi_ap_record_t *ap_info)` - Get scan result

## Configuration Parameters

### Default Values

| Parameter | Default | Description |
|-----------|---------|-------------|
| Max Retry | 5 | Maximum connection attempts |
| Timeout | 10000ms | Connection timeout |
| Auto Reconnect | true | Automatic reconnection |

### WiFi Settings

- **Mode**: Station (STA) mode only
- **Security**: WPA2-PSK authentication
- **PMF**: Capable but not required
- **Power Save**: Default ESP-IDF settings

## Error Handling

All functions return `esp_err_t` values:

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid arguments
- `ESP_ERR_INVALID_STATE` - Manager not initialized
- `ESP_ERR_NO_MEM` - Memory allocation failed
- `ESP_ERR_WIFI_CONN` - WiFi connection failed
- `ESP_ERR_TIMEOUT` - Connection timeout

## Event Callback System

The event callback function receives status updates and detailed information:

```c
typedef void (*wifi_event_callback_t)(wifi_status_t status, wifi_info_t *info);
```

**Callback Events:**
- Connection attempts and retries
- Successful connections with network details
- Disconnection events
- Connection failures and timeouts

## Threading and Concurrency

- **Thread Safe**: All functions can be called from any task
- **Event Handling**: Callbacks executed in WiFi event task context
- **Non-Blocking**: Connection functions use event groups for synchronization

## Memory Usage

- **RAM**: ~2KB for component data structures
- **Stack**: 4KB recommended for tasks using WiFi manager
- **Flash**: ~15KB compiled code size

## Integration Example

### Complete WiFi Test Implementation

```c
void wifi_test_task(void *pvParameters) {
    // Initialize
    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed");
        return;
    }
    
    // Set callback
    wifi_manager_set_callback(my_wifi_callback);
    
    // Configure and connect
    wifi_config_t config = {
        .ssid = "ros2_atom_ap",
        .password = "isolation-sphere",
        .max_retry = 5,
        .timeout_ms = 15000,
        .auto_reconnect = true
    };
    
    ret = wifi_manager_connect(&config);
    if (ret == ESP_OK) {
        // Monitor connection
        while (wifi_manager_is_connected()) {
            wifi_info_t info;
            wifi_manager_get_info(&info);
            ESP_LOGI(TAG, "WiFi OK: %s (%d dBm)", info.ssid, info.rssi);
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
    
    wifi_manager_deinit();
}
```

## Troubleshooting

### Common Issues

1. **Initialization fails**
   - Check NVS flash initialization
   - Verify event loop creation
   - Ensure sufficient heap memory

2. **Connection timeout**
   - Check SSID and password
   - Verify WiFi signal strength
   - Increase timeout value

3. **Frequent disconnections**
   - Check WiFi signal stability
   - Verify router compatibility
   - Monitor power supply stability

### Debug Logging

Enable verbose WiFi logging in `sdkconfig`:
```
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
CONFIG_ESP_WIFI_LOG_LEVEL_DEBUG=y
```

## Dependencies

- ESP-IDF WiFi component (`esp_wifi`)
- ESP-IDF Network Interface (`esp_netif`)
- ESP-IDF Event Library (`esp_event`)
- LwIP TCP/IP stack (`lwip`)
- FreeRTOS (`freertos`)

## Related Documentation

- [ESP-IDF WiFi Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/network/esp_wifi.html)
- [ESP-IDF Network Interface](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/network/esp_netif.html)
- Project test documentation: `tests/test03-wifi.md`