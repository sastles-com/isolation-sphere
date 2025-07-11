#include "wifi_test.hpp"
#include <cstring>
#include <cstdlib>

// Static instance for callback
WiFiTest* WiFiTest::instance_ = nullptr;

WiFiTest::WiFiTest()
    : BaseTest("WiFi", "WiFi connection and network functionality test"),
      connection_timeout_(15000),     // 15 seconds
      max_retries_(5),
      stability_test_duration_(30000), // 30 seconds
      auto_reconnect_(true),
      wifi_manager_initialized_(false),
      connected_successfully_(false),
      connection_attempts_(0),
      successful_connections_(0),
      connection_failures_(0),
      disconnection_events_(0),
      total_connection_time_(0),
      connection_start_time_(0),
      connection_in_progress_(false),
      last_status_(WIFI_STATUS_DISCONNECTED)
{
    // Default configuration (M5atomS3R test network)
    strcpy(wifi_config_.ssid, "ros2_atom_ap");
    strcpy(wifi_config_.password, "isolation-sphere");
    wifi_config_.max_retry = max_retries_;
    wifi_config_.timeout_ms = connection_timeout_;
    wifi_config_.auto_reconnect = auto_reconnect_;
    
    // Initialize connection info
    memset(&connection_info_, 0, sizeof(wifi_info_t));
    
    // Set static instance for callback
    instance_ = this;
}

esp_err_t WiFiTest::setup()
{
    logInfo("Setting up WiFi test environment");
    
    // Reset counters
    resetCounters();
    
    // Add test steps
    addStep("Initialize WiFi manager", [this]() { return initializeWiFiManager(); });
    addStep("Test WiFi connection", [this]() { return testWiFiConnection(); });
    addStep("Test connection stability", [this]() { return testConnectionStability(); });
    addStep("Test network scan", [this]() { return testNetworkScan(); });
    addStep("Test reconnection", [this]() { return testReconnection(); });
    addStep("Measure connection performance", [this]() { return measureConnectionPerformance(); });
    
    logPass("WiFi test setup completed");
    return ESP_OK;
}

esp_err_t WiFiTest::execute()
{
    logInfo("Executing WiFi test steps");
    
    // Log configuration
    logInfo("WiFi Configuration:");
    logInfo("  SSID: %s", wifi_config_.ssid);
    logInfo("  Max retries: %d", wifi_config_.max_retry);
    logInfo("  Timeout: %lu ms", wifi_config_.timeout_ms);
    logInfo("  Auto reconnect: %s", wifi_config_.auto_reconnect ? "enabled" : "disabled");
    
    // Run all test steps
    esp_err_t ret = runSteps();
    if (ret != ESP_OK) {
        logError("WiFi test execution failed");
        return ret;
    }
    
    // Log final statistics
    logConnectionStats();
    
    logPass("WiFi test execution completed successfully");
    return ESP_OK;
}

esp_err_t WiFiTest::teardown()
{
    logInfo("Cleaning up WiFi test");
    
    // Disconnect if connected
    if (wifi_manager_is_connected()) {
        logInfo("Disconnecting from WiFi");
        wifi_manager_disconnect();
    }
    
    // Note: WiFi manager deinit could be called here if needed
    // wifi_manager_deinit();
    
    wifi_manager_initialized_ = false;
    connected_successfully_ = false;
    
    // Clear static instance
    instance_ = nullptr;
    
    logPass("WiFi test cleanup completed");
    return ESP_OK;
}

esp_err_t WiFiTest::initializeWiFiManager()
{
    logInfo("Initializing WiFi manager");
    
    // Wait for other systems to initialize
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    esp_err_t ret = wifi_manager_init();
    TEST_ASSERT_OK(ret);
    
    wifi_manager_initialized_ = true;
    
    // Set event callback
    wifi_manager_set_callback(wifiEventCallback);
    
    logPass("WiFi manager initialized successfully");
    return ESP_OK;
}

esp_err_t WiFiTest::testWiFiConnection()
{
    logInfo("Testing WiFi connection to '%s'", wifi_config_.ssid);
    
    TEST_ASSERT(wifi_manager_initialized_, "WiFi manager must be initialized first");
    
    // Attempt connection
    connection_attempts_++;
    connection_in_progress_ = true;
    connection_start_time_ = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    esp_err_t ret = wifi_manager_connect(&wifi_config_);
    
    connection_in_progress_ = false;
    
    if (ret == ESP_OK) {
        successful_connections_++;
        connected_successfully_ = true;
        
        // Validate connection info
        ret = validateConnectionInfo();
        TEST_ASSERT_OK(ret);
        
        logPass("WiFi connection test passed");
    } else {
        connection_failures_++;
        logError("WiFi connection failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t WiFiTest::testConnectionStability()
{
    logInfo("Testing WiFi connection stability (%lu ms)", stability_test_duration_);
    
    TEST_ASSERT(connected_successfully_, "Must be connected to test stability");
    
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t check_count = 0;
    uint32_t disconnect_count = 0;
    
    while ((xTaskGetTickCount() * portTICK_PERIOD_MS - start_time) < stability_test_duration_) {
        check_count++;
        
        if (!wifi_manager_is_connected()) {
            disconnect_count++;
            logError("Unexpected disconnection detected (check %lu)", check_count);
        } else {
            // Get connection info
            wifi_info_t info;
            if (wifi_manager_get_info(&info) == ESP_OK) {
                // Log RSSI occasionally
                if (check_count % 10 == 0) {
                    logInfo("Stability check %lu: RSSI=%d dBm, IP=%s", 
                            check_count, info.rssi, info.ip_addr);
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));  // Check every second
    }
    
    float stability_rate = (float)(check_count - disconnect_count) / check_count * 100.0f;
    
    logInfo("Stability test results:");
    logInfo("  Total checks: %lu", check_count);
    logInfo("  Disconnections: %lu", disconnect_count);
    logInfo("  Stability rate: %.1f%%", stability_rate);
    
    // Require at least 95% stability
    TEST_ASSERT(stability_rate >= 95.0f, "Connection stability below acceptable threshold");
    
    logPass("Connection stability test passed");
    return ESP_OK;
}

esp_err_t WiFiTest::testNetworkScan()
{
    logInfo("Testing WiFi network scan");
    
    TEST_ASSERT(wifi_manager_initialized_, "WiFi manager must be initialized first");
    
    // Start scan
    esp_err_t ret = wifi_manager_scan_start();
    TEST_ASSERT_OK(ret);
    
    // Wait for scan to complete
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Get scan results
    uint16_t scan_count = wifi_manager_get_scan_count();
    logInfo("Found %d WiFi networks", scan_count);
    
    TEST_ASSERT(scan_count > 0, "No WiFi networks found in scan");
    
    // Look for our target network
    bool target_found = false;
    for (uint16_t i = 0; i < scan_count && i < 10; i++) {  // Limit to first 10
        wifi_ap_record_t ap_info;
        if (wifi_manager_get_scan_result(i, &ap_info) == ESP_OK) {
            logInfo("Network %d: SSID=%s, RSSI=%d dBm, Channel=%d", 
                    i + 1, (char*)ap_info.ssid, ap_info.rssi, ap_info.primary);
            
            if (strcmp((char*)ap_info.ssid, wifi_config_.ssid) == 0) {
                target_found = true;
                logPass("Target network '%s' found with RSSI=%d dBm", 
                        wifi_config_.ssid, ap_info.rssi);
            }
        }
    }
    
    if (!target_found) {
        logError("Target network '%s' not found in scan results", wifi_config_.ssid);
        // Don't fail the test - network might be temporarily unavailable
    }
    
    logPass("Network scan test completed");
    return ESP_OK;
}

esp_err_t WiFiTest::testReconnection()
{
    logInfo("Testing WiFi reconnection capability");
    
    TEST_ASSERT(connected_successfully_, "Must be connected to test reconnection");
    
    // Disconnect
    logInfo("Disconnecting from WiFi");
    esp_err_t ret = wifi_manager_disconnect();
    TEST_ASSERT_OK(ret);
    
    // Wait for disconnection to complete
    uint32_t disconnect_timeout = 5000;  // 5 seconds
    uint32_t disconnect_start = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while (wifi_manager_is_connected()) {
        if ((xTaskGetTickCount() * portTICK_PERIOD_MS - disconnect_start) > disconnect_timeout) {
            logError("Timeout waiting for disconnection");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    TEST_ASSERT(!wifi_manager_is_connected(), "Should be disconnected");
    
    // Reconnect
    logInfo("Attempting reconnection");
    connection_attempts_++;
    ret = wifi_manager_connect(&wifi_config_);
    
    if (ret == ESP_OK) {
        successful_connections_++;
        logPass("Reconnection test passed");
    } else {
        connection_failures_++;
        logError("Reconnection failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t WiFiTest::measureConnectionPerformance()
{
    logInfo("Measuring WiFi connection performance");
    
    TEST_ASSERT(wifi_manager_is_connected(), "Must be connected to measure performance");
    
    // Get detailed connection info
    wifi_info_t info;
    esp_err_t ret = wifi_manager_get_info(&info);
    TEST_ASSERT_OK(ret);
    
    logConnectionInfo(&info);
    
    // Test basic network performance (could be expanded)
    ret = testNetworkPerformance();
    if (ret != ESP_OK) {
        logError("Network performance test failed: %s", esp_err_to_name(ret));
        // Don't fail the entire test for performance issues
    }
    
    logPass("Connection performance measurement completed");
    return ESP_OK;
}

void WiFiTest::setNetworkCredentials(const std::string& ssid, const std::string& password)
{
    strncpy(wifi_config_.ssid, ssid.c_str(), sizeof(wifi_config_.ssid) - 1);
    strncpy(wifi_config_.password, password.c_str(), sizeof(wifi_config_.password) - 1);
    wifi_config_.ssid[sizeof(wifi_config_.ssid) - 1] = '\0';
    wifi_config_.password[sizeof(wifi_config_.password) - 1] = '\0';
}

void WiFiTest::wifiEventCallback(wifi_status_t status, wifi_info_t *info)
{
    if (instance_) {
        instance_->handleWiFiEvent(status, info);
    }
}

void WiFiTest::handleWiFiEvent(wifi_status_t status, wifi_info_t *info)
{
    last_status_ = status;
    
    switch (status) {
        case WIFI_STATUS_CONNECTING:
            logInfo("WiFi event: Connecting... (retry: %d)", info ? info->retry_count : 0);
            break;
            
        case WIFI_STATUS_CONNECTED:
            logInfo("WiFi event: Connected successfully");
            if (info) {
                connection_info_ = *info;
                logConnectionInfo(info);
            }
            
            if (connection_in_progress_) {
                uint32_t connection_time = xTaskGetTickCount() * portTICK_PERIOD_MS - connection_start_time_;
                total_connection_time_ += connection_time;
                logInfo("Connection established in %lu ms", connection_time);
            }
            break;
            
        case WIFI_STATUS_DISCONNECTED:
            logInfo("WiFi event: Disconnected");
            disconnection_events_++;
            break;
            
        case WIFI_STATUS_FAILED:
            logError("WiFi event: Connection failed");
            break;
            
        case WIFI_STATUS_TIMEOUT:
            logError("WiFi event: Connection timeout");
            break;
            
        default:
            logError("WiFi event: Unknown status %d", status);
            break;
    }
}

esp_err_t WiFiTest::waitForConnection()
{
    uint32_t timeout = connection_timeout_;
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    while (!wifi_manager_is_connected()) {
        if ((xTaskGetTickCount() * portTICK_PERIOD_MS - start_time) > timeout) {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    return ESP_OK;
}

esp_err_t WiFiTest::validateConnectionInfo()
{
    wifi_info_t info;
    esp_err_t ret = wifi_manager_get_info(&info);
    TEST_ASSERT_OK(ret);
    
    // Validate SSID
    TEST_ASSERT(strlen(info.ssid) > 0, "SSID should not be empty");
    TEST_ASSERT(strcmp(info.ssid, wifi_config_.ssid) == 0, "Connected SSID mismatch");
    
    // Validate IP address
    TEST_ASSERT(isValidIP(info.ip_addr), "Invalid IP address");
    
    // Validate RSSI (reasonable range) - convert to signed if needed
    int8_t rssi_signed = (int8_t)info.rssi;
    if (rssi_signed > 0) {
        // Likely unsigned interpretation, convert
        rssi_signed = info.rssi - 256;
    }
    TEST_ASSERT(rssi_signed >= -100 && rssi_signed <= 0, "RSSI out of reasonable range");
    
    connection_info_ = info;
    return ESP_OK;
}

esp_err_t WiFiTest::testNetworkPerformance()
{
    // Basic performance test - could be expanded with ping, throughput tests, etc.
    logInfo("Basic network performance check");
    
    // For now, just verify we can get connection info quickly
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    wifi_info_t info;
    esp_err_t ret = wifi_manager_get_info(&info);
    
    uint32_t info_time = xTaskGetTickCount() * portTICK_PERIOD_MS - start_time;
    
    if (ret == ESP_OK) {
        logInfo("Connection info retrieval time: %lu ms", info_time);
        TEST_ASSERT(info_time < 1000, "Connection info retrieval too slow");
    }
    
    return ret;
}

void WiFiTest::resetCounters()
{
    connection_attempts_ = 0;
    successful_connections_ = 0;
    connection_failures_ = 0;
    disconnection_events_ = 0;
    total_connection_time_ = 0;
    connected_successfully_ = false;
    connection_in_progress_ = false;
    memset(&connection_info_, 0, sizeof(wifi_info_t));
}

void WiFiTest::logConnectionInfo(const wifi_info_t* info)
{
    if (!info) return;
    
    logInfo("Connection Info:");
    logInfo("  SSID: %s", info->ssid);
    logInfo("  IP Address: %s", info->ip_addr);
    logInfo("  Gateway: %s", info->gateway);
    logInfo("  Netmask: %s", info->netmask);
    logInfo("  RSSI: %d dBm", info->rssi);
    logInfo("  Channel: %d", info->channel);
    logInfo("  Connection time: %lu ms", info->connection_time_ms);
}

void WiFiTest::logConnectionStats()
{
    logInfo("WiFi Test Statistics:");
    logInfo("  Connection attempts: %lu", connection_attempts_);
    logInfo("  Successful connections: %lu", successful_connections_);
    logInfo("  Connection failures: %lu", connection_failures_);
    logInfo("  Disconnection events: %lu", disconnection_events_);
    
    if (successful_connections_ > 0) {
        float avg_connection_time = (float)total_connection_time_ / successful_connections_;
        logInfo("  Average connection time: %.1f ms", avg_connection_time);
        
        float success_rate = (float)successful_connections_ / connection_attempts_ * 100.0f;
        logInfo("  Success rate: %.1f%%", success_rate);
    }
}

bool WiFiTest::isValidIP(const char* ip_str)
{
    if (!ip_str || strlen(ip_str) == 0) {
        return false;
    }
    
    // Simple IP validation - check for format like "192.168.1.100"
    int octets[4];
    int count = sscanf(ip_str, "%d.%d.%d.%d", &octets[0], &octets[1], &octets[2], &octets[3]);
    
    if (count != 4) {
        return false;
    }
    
    for (int i = 0; i < 4; i++) {
        if (octets[i] < 0 || octets[i] > 255) {
            return false;
        }
    }
    
    return true;
}