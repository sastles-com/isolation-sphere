#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

// WiFi configuration constants
#define WIFI_MANAGER_SSID_MAX_LEN       32
#define WIFI_MANAGER_PASSWORD_MAX_LEN   64
#define WIFI_MANAGER_MAX_RETRY          5
#define WIFI_MANAGER_CONNECT_TIMEOUT_MS 10000

// WiFi connection status
typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_FAILED,
    WIFI_STATUS_TIMEOUT
} wifi_status_t;

// WiFi configuration structure
typedef struct {
    char ssid[WIFI_MANAGER_SSID_MAX_LEN];
    char password[WIFI_MANAGER_PASSWORD_MAX_LEN];
    uint8_t max_retry;
    uint32_t timeout_ms;
    bool auto_reconnect;
} wifi_manager_config_t;

// WiFi connection information
typedef struct {
    wifi_status_t status;
    char ssid[WIFI_MANAGER_SSID_MAX_LEN];
    uint8_t rssi;
    uint8_t channel;
    char ip_addr[16];
    char gateway[16];
    char netmask[16];
    uint32_t connection_time_ms;
    uint8_t retry_count;
} wifi_info_t;

// Event callback function type
typedef void (*wifi_event_callback_t)(wifi_status_t status, wifi_info_t *info);

// Function prototypes

/**
 * @brief Initialize WiFi manager
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Deinitialize WiFi manager
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_deinit(void);

/**
 * @brief Connect to WiFi network
 * 
 * @param config WiFi configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_connect(const wifi_manager_config_t *config);

/**
 * @brief Disconnect from WiFi network
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Get current WiFi status
 * 
 * @return wifi_status_t Current connection status
 */
wifi_status_t wifi_manager_get_status(void);

/**
 * @brief Get WiFi connection information
 * 
 * @param info Pointer to store WiFi information
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_get_info(wifi_info_t *info);

/**
 * @brief Check if WiFi is connected
 * 
 * @return true if connected, false otherwise
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Set event callback function
 * 
 * @param callback Callback function for WiFi events
 */
void wifi_manager_set_callback(wifi_event_callback_t callback);

/**
 * @brief Start WiFi scan
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_scan_start(void);

/**
 * @brief Get scan results count
 * 
 * @return uint16_t Number of found networks
 */
uint16_t wifi_manager_get_scan_count(void);

/**
 * @brief Get scan result
 * 
 * @param index Index of scan result
 * @param ap_info Pointer to store AP information
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_get_scan_result(uint16_t index, wifi_ap_record_t *ap_info);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H