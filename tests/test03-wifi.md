# Test03: WiFi Connection Test

## Overview

Verification of M5atomS3R's WiFi connectivity functionality including station mode initialization, network connection, and status monitoring using the target network "ros2_atom_ap".

## Test Objectives

- [x] Verify WiFi hardware initialization
- [x] Test station mode configuration
- [x] Validate connection to specific SSID "ros2_atom_ap"
- [x] Confirm network parameter acquisition (IP, gateway, DNS)
- [x] Test connection stability and monitoring
- [x] Verify automatic reconnection capability

## Hardware Requirements

- M5atomS3R device with WiFi capability
- Access to WiFi network "ros2_atom_ap"
- USB-C cable for power and programming
- Host computer with ESP-IDF development environment

## Network Requirements

### Target WiFi Network

| Parameter | Value |
|-----------|-------|
| **SSID** | `ros2_atom_ap` |
| **Password** | `isolation-sphere` |
| **Security** | WPA2-PSK (recommended) |
| **Frequency** | 2.4 GHz |
| **Signal Strength** | > -70 dBm (recommended) |

### Network Configuration

- DHCP enabled for automatic IP assignment
- Internet connectivity (optional for basic test)
- Stable signal coverage in test area

## Test Configuration

### WiFi Manager Settings

```c
wifi_config_t config = {
    .ssid = "ros2_atom_ap",
    .password = "isolation-sphere",
    .max_retry = 5,
    .timeout_ms = 15000,  // 15 seconds
    .auto_reconnect = true
};
```

### Connection Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| SSID | ros2_atom_ap | Target network name |
| Password | isolation-sphere | Network authentication |
| Max Retries | 5 | Connection attempt limit |
| Timeout | 15 seconds | Per-attempt timeout |
| Auto Reconnect | Enabled | Automatic reconnection |

## Test Procedure

### 1. Network Preparation

1. Ensure WiFi network "ros2_atom_ap" is active and broadcasting
2. Verify password "isolation-sphere" is correct
3. Check signal strength at test location
4. Confirm DHCP is enabled on the network

### 2. Device Preparation

```bash
# Build and flash with WiFi support
./dev.sh build-flash

# Start monitoring with auto-reconnect
./monitor-loop.sh
```

### 3. Test Execution

1. Reset M5atomS3R device
2. Monitor initialization sequence
3. Observe WiFi connection attempts
4. Verify successful connection and network information
5. Monitor stability over time

### 4. Connection Verification

1. Check IP address assignment
2. Verify gateway and netmask configuration
3. Test signal strength reporting
4. Confirm automatic reconnection (if disconnected)

## Expected Results

### Successful Initialization

```
I (xxxx) M5ATOMS3R: WiFi test task started
I (xxxx) WIFI_MANAGER: Initializing WiFi manager...
I (xxxx) WIFI_MANAGER: WiFi manager initialized successfully
I (xxxx) M5ATOMS3R: WiFi manager initialized successfully
```

### Connection Sequence

```
I (xxxx) M5ATOMS3R: === WiFi Connection Test ===
I (xxxx) M5ATOMS3R: Target SSID: ros2_atom_ap
I (xxxx) M5ATOMS3R: Max retries: 5
I (xxxx) M5ATOMS3R: Timeout: 15000 ms
I (xxxx) M5ATOMS3R: ===========================
I (xxxx) M5ATOMS3R: Attempting to connect to WiFi...
I (xxxx) WIFI_MANAGER: Connecting to WiFi SSID: ros2_atom_ap
I (xxxx) WIFI_MANAGER: WiFi station started
```

### Successful Connection

```
I (xxxx) WIFI_MANAGER: Connected to WiFi SSID: ros2_atom_ap
I (xxxx) WIFI_MANAGER: Got IP address: 192.168.1.xxx
I (xxxx) M5ATOMS3R: === WiFi Connected Successfully ===
I (xxxx) M5ATOMS3R: SSID: ros2_atom_ap
I (xxxx) M5ATOMS3R: IP Address: 192.168.1.xxx
I (xxxx) M5ATOMS3R: Gateway: 192.168.1.1
I (xxxx) M5ATOMS3R: Netmask: 255.255.255.0
I (xxxx) M5ATOMS3R: RSSI: -45 dBm
I (xxxx) M5ATOMS3R: Channel: 6
I (xxxx) M5ATOMS3R: Connection time: 3245 ms
I (xxxx) M5ATOMS3R: ==================================
I (xxxx) M5ATOMS3R: WiFi connection successful!
```

### Status Monitoring

```
I (xxxx) M5ATOMS3R: WiFi Status: Connected to ros2_atom_ap (RSSI: -45 dBm, IP: 192.168.1.xxx)
I (xxxx) M5ATOMS3R: WiFi Status: Connected to ros2_atom_ap (RSSI: -47 dBm, IP: 192.168.1.xxx)
I (xxxx) M5ATOMS3R: WiFi Status: Connected to ros2_atom_ap (RSSI: -44 dBm, IP: 192.168.1.xxx)
```

## Success Criteria

| Criteria | Expected Value | Validation |
|----------|----------------|------------|
| WiFi Initialization | Success | No initialization errors |
| Network Detection | "ros2_atom_ap" found | SSID appears in scan results |
| Authentication | Success | Correct password accepted |
| IP Assignment | Valid IP address | DHCP allocation successful |
| Connection Time | < 15 seconds | Within timeout limit |
| Signal Strength | > -70 dBm | Adequate signal quality |
| Status Monitoring | Continuous updates | Regular status reports |
| Auto Reconnection | Functional | Reconnects after disconnection |

## Error Scenarios

### 1. Network Not Found

```
E (xxxx) WIFI_MANAGER: WiFi disconnected, reason: 201
E (xxxx) WIFI_MANAGER: Failed to connect to WiFi after 5 retries
E (xxxx) M5ATOMS3R: WiFi: Connection failed after 5 retries
E (xxxx) M5ATOMS3R: WiFi connection failed: ESP_ERR_WIFI_CONN
E (xxxx) M5ATOMS3R: Please check:
E (xxxx) M5ATOMS3R: 1. WiFi SSID 'ros2_atom_ap' is available
E (xxxx) M5ATOMS3R: 2. Password 'isolation-sphere' is correct
E (xxxx) M5ATOMS3R: 3. WiFi signal strength is sufficient
```

### 2. Authentication Failure

```
E (xxxx) WIFI_MANAGER: WiFi disconnected, reason: 15
E (xxxx) M5ATOMS3R: WiFi: Connection failed after 5 retries
```

### 3. Connection Timeout

```
E (xxxx) WIFI_MANAGER: Connection timeout for WiFi SSID: ros2_atom_ap
E (xxxx) M5ATOMS3R: WiFi: Connection timeout
E (xxxx) M5ATOMS3R: WiFi connection failed: ESP_ERR_TIMEOUT
```

## Troubleshooting

### Network Issues

1. **SSID not found**
   - Verify network is broadcasting
   - Check 2.4GHz frequency (ESP32-S3 limitation)
   - Confirm network is not hidden
   - Check for special characters in SSID

2. **Authentication failure**
   - Verify password "isolation-sphere" is correct
   - Check for case sensitivity
   - Confirm WPA2-PSK security mode
   - Test with simplified password

3. **Weak signal**
   - Move closer to access point
   - Check for interference sources
   - Verify antenna connections
   - Use WiFi analyzer to check signal

### Device Issues

1. **WiFi initialization failure**
   - Check NVS flash initialization
   - Verify memory availability
   - Confirm ESP-IDF WiFi component enabled

2. **Memory issues**
   - Monitor heap usage during connection
   - Check for memory leaks
   - Verify task stack sizes

3. **Timing issues**
   - Increase connection timeout
   - Add delays between attempts
   - Check task scheduling

### Resolution Steps

```bash
# Clean rebuild
./docker-build.sh fullclean
./dev.sh build-flash

# Network verification
# - Scan for available networks
# - Verify signal strength
# - Test with mobile device

# Debug WiFi communication
# - Enable verbose WiFi logging
# - Monitor connection events
# - Check for interference
```

## Component Architecture

### WiFi Manager Component

```
components/wifi_manager/
├── include/wifi_manager.h    # Public API
├── src/wifi_manager.c       # Implementation
├── CMakeLists.txt           # Build configuration
└── README.md                # Component documentation
```

### Key API Functions

```c
// Initialization
esp_err_t wifi_manager_init(void);
esp_err_t wifi_manager_deinit(void);

// Connection management
esp_err_t wifi_manager_connect(const wifi_config_t *config);
esp_err_t wifi_manager_disconnect(void);

// Status monitoring
wifi_status_t wifi_manager_get_status(void);
esp_err_t wifi_manager_get_info(wifi_info_t *info);
bool wifi_manager_is_connected(void);

// Event handling
void wifi_manager_set_callback(wifi_event_callback_t callback);
```

### Event Callback System

```c
void wifi_event_callback(wifi_status_t status, wifi_info_t *info) {
    switch (status) {
        case WIFI_STATUS_CONNECTING:
            // Connection in progress
            break;
        case WIFI_STATUS_CONNECTED:
            // Successfully connected
            break;
        case WIFI_STATUS_DISCONNECTED:
            // Connection lost
            break;
        case WIFI_STATUS_FAILED:
            // Connection failed
            break;
        case WIFI_STATUS_TIMEOUT:
            // Connection timeout
            break;
    }
}
```

## Test Implementation

### Main Test Task

Location: `main/main.cpp:259-333`

```cpp
void wifi_test_task(void *pvParameters)
{
    // Initialize WiFi manager
    esp_err_t ret = wifi_manager_init();
    wifi_manager_set_callback(wifi_event_callback);
    
    // Configure target network
    wifi_config_t config = {
        .ssid = "ros2_atom_ap",
        .password = "isolation-sphere",
        .max_retry = 5,
        .timeout_ms = 15000,
        .auto_reconnect = true
    };
    
    // Attempt connection
    ret = wifi_manager_connect(&config);
    
    if (ret == ESP_OK) {
        // Monitor connection status
        while (wifi_manager_is_connected()) {
            wifi_info_t info;
            wifi_manager_get_info(&info);
            ESP_LOGI(TAG, "WiFi Status: Connected to %s (RSSI: %d dBm, IP: %s)", 
                     info.ssid, info.rssi, info.ip_addr);
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
}
```

## Performance Characteristics

### Timing

- **Initialization**: ~1-2 seconds
- **Connection Time**: 3-10 seconds (typical)
- **Reconnection**: 5-15 seconds
- **Status Update**: 100ms response time

### Network Parameters

- **Frequency**: 2.4 GHz only (ESP32-S3 limitation)
- **Security**: WPA/WPA2-PSK, WPA3 (depending on ESP-IDF version)
- **Power Management**: Modem sleep enabled by default
- **Range**: Typical indoor WiFi range

### Memory Usage

- **WiFi Manager**: ~2KB RAM
- **ESP-IDF WiFi Stack**: ~40KB RAM
- **Task Stack**: 4KB recommended
- **Flash Size**: ~20KB compiled code

## Test Status

**Status**: 🔧 READY FOR EXECUTION  
**Implementation**: ✅ COMPLETE  
**Network Required**: ros2_atom_ap with password "isolation-sphere"  
**Expected Duration**: 5-10 minutes setup + continuous monitoring  

## Security Considerations

- Password stored in plain text (development only)
- WPA2-PSK minimum security requirement
- No certificate validation for enterprise networks
- Consider encrypted storage for production use

## Related Documentation

- [WiFi Manager Component README](../components/wifi_manager/README.md)
- [ESP-IDF WiFi Driver Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/network/esp_wifi.html)
- [ESP32-S3 WiFi Specifications](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- Project main documentation: `CLAUDE.md`

## Notes

- WiFi connection is persistent across device resets
- RSSI values are updated periodically during monitoring
- Gateway and DNS information automatically configured via DHCP
- Connection stability depends on signal strength and network congestion
- Auto-reconnection handles temporary network outages
- Status monitoring provides real-time connection health information