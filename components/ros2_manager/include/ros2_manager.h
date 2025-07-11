#ifndef ROS2_MANAGER_H
#define ROS2_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

// ROS2 configuration constants
#define ROS2_MANAGER_NODE_NAME_MAX_LEN      64
#define ROS2_MANAGER_TOPIC_NAME_MAX_LEN     128
#define ROS2_MANAGER_FRAME_BUFFER_SIZE      32768  // 32KB for JPEG frames
#define ROS2_MANAGER_MAX_MESSAGE_SIZE       1024

// ROS2 connection status
typedef enum {
    ROS2_STATUS_DISCONNECTED = 0,
    ROS2_STATUS_CONNECTING,
    ROS2_STATUS_CONNECTED,
    ROS2_STATUS_PUBLISHING,
    ROS2_STATUS_SUBSCRIBING,
    ROS2_STATUS_ERROR,
    ROS2_STATUS_TIMEOUT
} ros2_status_t;

// IMU data structure (matches sensor_msgs/Imu)
typedef struct {
    // Header
    uint32_t seq;
    uint64_t timestamp_ns;
    char frame_id[32];
    
    // Orientation (quaternion)
    float orientation_x;
    float orientation_y;
    float orientation_z;
    float orientation_w;
    float orientation_covariance[9];
    
    // Angular velocity
    float angular_velocity_x;
    float angular_velocity_y;
    float angular_velocity_z;
    float angular_velocity_covariance[9];
    
    // Linear acceleration
    float linear_acceleration_x;
    float linear_acceleration_y;
    float linear_acceleration_z;
    float linear_acceleration_covariance[9];
} ros2_imu_msg_t;

// Compressed image data structure (matches sensor_msgs/CompressedImage)
typedef struct {
    // Header
    uint32_t seq;
    uint64_t timestamp_ns;
    char frame_id[32];
    
    // Image data
    char format[16];        // e.g., "jpeg"
    uint8_t* data;          // Compressed image data
    size_t data_size;       // Size of compressed data
} ros2_compressed_image_msg_t;

// ROS2 configuration structure
typedef struct {
    char node_name[ROS2_MANAGER_NODE_NAME_MAX_LEN];
    char imu_topic[ROS2_MANAGER_TOPIC_NAME_MAX_LEN];
    char image_topic[ROS2_MANAGER_TOPIC_NAME_MAX_LEN];
    uint32_t publish_rate_hz;
    uint32_t connection_timeout_ms;
    bool auto_reconnect;
} ros2_manager_config_t;

// ROS2 statistics
typedef struct {
    uint32_t messages_published;
    uint32_t messages_received;
    uint32_t publish_errors;
    uint32_t receive_errors;
    uint32_t connection_attempts;
    uint32_t successful_connections;
    uint32_t disconnection_events;
    uint32_t total_uptime_ms;
} ros2_statistics_t;

// Event callback function types
typedef void (*ros2_connection_callback_t)(ros2_status_t status);
typedef void (*ros2_image_callback_t)(const ros2_compressed_image_msg_t* image);
typedef void (*ros2_error_callback_t)(esp_err_t error, const char* message);

// Function prototypes

/**
 * @brief Initialize ROS2 manager
 * 
 * @param config ROS2 configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ros2_manager_init(const ros2_manager_config_t* config);

/**
 * @brief Deinitialize ROS2 manager
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ros2_manager_deinit(void);

/**
 * @brief Start ROS2 communication
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ros2_manager_start(void);

/**
 * @brief Stop ROS2 communication
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ros2_manager_stop(void);

/**
 * @brief Publish IMU data
 * 
 * @param imu_data IMU message data
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ros2_manager_publish_imu(const ros2_imu_msg_t* imu_data);

/**
 * @brief Check if ROS2 is connected
 * 
 * @return true if connected, false otherwise
 */
bool ros2_manager_is_connected(void);

/**
 * @brief Get current ROS2 status
 * 
 * @return ros2_status_t Current status
 */
ros2_status_t ros2_manager_get_status(void);

/**
 * @brief Get ROS2 statistics
 * 
 * @param stats Pointer to store statistics
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ros2_manager_get_statistics(ros2_statistics_t* stats);

/**
 * @brief Set connection status callback
 * 
 * @param callback Callback function for connection status changes
 */
void ros2_manager_set_connection_callback(ros2_connection_callback_t callback);

/**
 * @brief Set image receive callback
 * 
 * @param callback Callback function for received images
 */
void ros2_manager_set_image_callback(ros2_image_callback_t callback);

/**
 * @brief Set error callback
 * 
 * @param callback Callback function for errors
 */
void ros2_manager_set_error_callback(ros2_error_callback_t callback);

/**
 * @brief Convert BNO055 quaternion to ROS2 IMU message
 * 
 * @param quat BNO055 quaternion data
 * @param imu_msg Output ROS2 IMU message
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ros2_manager_bno055_to_imu_msg(const void* quat, ros2_imu_msg_t* imu_msg);

/**
 * @brief Get status string representation
 * 
 * @param status ROS2 status
 * @return const char* Status string
 */
const char* ros2_manager_status_to_string(ros2_status_t status);

// Mock implementation functions (for testing without actual ROS2)
/**
 * @brief Enable mock mode for testing
 * 
 * @param enable True to enable mock mode
 */
void ros2_manager_set_mock_mode(bool enable);

/**
 * @brief Simulate receiving an image in mock mode
 * 
 * @param image Mock image data
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ros2_manager_mock_receive_image(const ros2_compressed_image_msg_t* image);

#ifdef __cplusplus
}
#endif

#endif // ROS2_MANAGER_H