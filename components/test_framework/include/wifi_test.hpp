#ifndef WIFI_TEST_HPP
#define WIFI_TEST_HPP

#include "base_test.hpp"
#include "wifi_manager.h"
#include <string>

class WiFiTest : public BaseTest {
public:
    WiFiTest();
    virtual ~WiFiTest() = default;

    // Implement base test methods
    esp_err_t setup() override;
    esp_err_t execute() override;
    esp_err_t teardown() override;

    // WiFi-specific methods
    esp_err_t initializeWiFiManager();
    esp_err_t testWiFiConnection();
    esp_err_t testConnectionStability();
    esp_err_t testNetworkScan();
    esp_err_t testReconnection();
    esp_err_t measureConnectionPerformance();

    // Configuration
    void setNetworkCredentials(const std::string& ssid, const std::string& password);
    void setConnectionTimeout(uint32_t timeout_ms) { connection_timeout_ = timeout_ms; }
    void setMaxRetries(uint8_t max_retries) { max_retries_ = max_retries; }
    void setStabilityTestDuration(uint32_t duration_ms) { stability_test_duration_ = duration_ms; }
    void setAutoReconnect(bool auto_reconnect) { auto_reconnect_ = auto_reconnect; }

private:
    // Configuration
    wifi_manager_config_t wifi_config_;
    uint32_t connection_timeout_;
    uint8_t max_retries_;
    uint32_t stability_test_duration_;
    bool auto_reconnect_;
    
    // Test state
    bool wifi_manager_initialized_;
    bool connected_successfully_;
    
    // Test results
    wifi_info_t connection_info_;
    uint32_t connection_attempts_;
    uint32_t successful_connections_;
    uint32_t connection_failures_;
    uint32_t disconnection_events_;
    uint32_t total_connection_time_;
    
    // Event handling
    static WiFiTest* instance_;  // For static callback
    static void wifiEventCallback(wifi_status_t status, wifi_info_t *info);
    void handleWiFiEvent(wifi_status_t status, wifi_info_t *info);
    
    // Helper methods
    esp_err_t waitForConnection();
    esp_err_t validateConnectionInfo();
    esp_err_t testNetworkPerformance();
    void resetCounters();
    void logConnectionInfo(const wifi_info_t* info);
    void logConnectionStats();
    bool isValidIP(const char* ip_str);
    
    // Connection monitoring
    uint32_t connection_start_time_;
    bool connection_in_progress_;
    wifi_status_t last_status_;
};

#endif // WIFI_TEST_HPP