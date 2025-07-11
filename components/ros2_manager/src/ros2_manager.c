#include "ros2_manager.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include <string.h>
#include <math.h>

static const char *TAG = "ROS2_MANAGER";

// Global state
static bool ros2_manager_initialized = false;
static bool ros2_manager_started = false;
static bool ros2_mock_mode = false;
static ros2_status_t current_status = ROS2_STATUS_DISCONNECTED;
static ros2_manager_config_t current_config = {0};
static ros2_statistics_t current_stats = {0};

// Callbacks
static ros2_connection_callback_t connection_callback = NULL;
static ros2_image_callback_t image_callback = NULL;
static ros2_error_callback_t error_callback = NULL;

// Task handles and queues
static TaskHandle_t publish_task_handle = NULL;
static TaskHandle_t subscribe_task_handle = NULL;
static QueueHandle_t imu_publish_queue = NULL;
static TimerHandle_t connection_timer = NULL;

// Internal state
static uint32_t initialization_time = 0;
static uint32_t last_publish_time = 0;
static uint32_t sequence_number = 0;

// Forward declarations
static void publish_task(void *pvParameters);
static void subscribe_task(void *pvParameters);
static void connection_timer_callback(TimerHandle_t xTimer);
static void notify_status_change(ros2_status_t new_status);
static void notify_error(esp_err_t error, const char* message);
static esp_err_t simulate_ros2_connection(void);
static esp_err_t simulate_ros2_publish(const ros2_imu_msg_t* imu_data);
static void simulate_image_reception(void);

esp_err_t ros2_manager_init(const ros2_manager_config_t* config)
{
    if (ros2_manager_initialized) {
        ESP_LOGW(TAG, "ROS2 manager already initialized");
        return ESP_OK;
    }
    
    if (!config) {
        ESP_LOGE(TAG, "Configuration cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Initializing ROS2 manager");
    ESP_LOGI(TAG, "Node name: %s", config->node_name);
    ESP_LOGI(TAG, "IMU topic: %s", config->imu_topic);
    ESP_LOGI(TAG, "Image topic: %s", config->image_topic);
    ESP_LOGI(TAG, "Publish rate: %lu Hz", config->publish_rate_hz);
    
    // Copy configuration
    memcpy(&current_config, config, sizeof(ros2_manager_config_t));
    
    // Create IMU publish queue
    imu_publish_queue = xQueueCreate(10, sizeof(ros2_imu_msg_t));
    if (!imu_publish_queue) {
        ESP_LOGE(TAG, "Failed to create IMU publish queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Create connection timer
    connection_timer = xTimerCreate("ros2_conn_timer",
                                   pdMS_TO_TICKS(current_config.connection_timeout_ms),
                                   pdFALSE,  // One-shot timer
                                   NULL,
                                   connection_timer_callback);
    if (!connection_timer) {
        ESP_LOGE(TAG, "Failed to create connection timer");
        vQueueDelete(imu_publish_queue);
        return ESP_ERR_NO_MEM;
    }
    
    // Reset statistics
    memset(&current_stats, 0, sizeof(ros2_statistics_t));
    initialization_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Initialize status
    current_status = ROS2_STATUS_DISCONNECTED;
    ros2_manager_initialized = true;
    
    ESP_LOGI(TAG, "ROS2 manager initialized successfully");
    return ESP_OK;
}

esp_err_t ros2_manager_deinit(void)
{
    if (!ros2_manager_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deinitializing ROS2 manager");
    
    // Stop if running
    ros2_manager_stop();
    
    // Delete queues and timers
    if (imu_publish_queue) {
        vQueueDelete(imu_publish_queue);
        imu_publish_queue = NULL;
    }
    
    if (connection_timer) {
        xTimerDelete(connection_timer, portMAX_DELAY);
        connection_timer = NULL;
    }
    
    // Reset state
    ros2_manager_initialized = false;
    current_status = ROS2_STATUS_DISCONNECTED;
    
    ESP_LOGI(TAG, "ROS2 manager deinitialized");
    return ESP_OK;
}

esp_err_t ros2_manager_start(void)
{
    if (!ros2_manager_initialized) {
        ESP_LOGE(TAG, "ROS2 manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (ros2_manager_started) {
        ESP_LOGW(TAG, "ROS2 manager already started");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting ROS2 manager");
    
    // Set connecting status
    notify_status_change(ROS2_STATUS_CONNECTING);
    
    // Create publish task
    if (xTaskCreate(publish_task, "ros2_publish", 4096, NULL, 5, &publish_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create publish task");
        return ESP_ERR_NO_MEM;
    }
    
    // Create subscribe task
    if (xTaskCreate(subscribe_task, "ros2_subscribe", 4096, NULL, 4, &subscribe_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create subscribe task");
        vTaskDelete(publish_task_handle);
        publish_task_handle = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    // Start connection timer
    xTimerStart(connection_timer, 0);
    
    ros2_manager_started = true;
    current_stats.connection_attempts++;
    
    ESP_LOGI(TAG, "ROS2 manager started successfully");
    return ESP_OK;
}

esp_err_t ros2_manager_stop(void)
{
    if (!ros2_manager_started) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Stopping ROS2 manager");
    
    // Stop timer
    if (connection_timer) {
        xTimerStop(connection_timer, portMAX_DELAY);
    }
    
    // Delete tasks
    if (publish_task_handle) {
        vTaskDelete(publish_task_handle);
        publish_task_handle = NULL;
    }
    
    if (subscribe_task_handle) {
        vTaskDelete(subscribe_task_handle);
        subscribe_task_handle = NULL;
    }
    
    ros2_manager_started = false;
    notify_status_change(ROS2_STATUS_DISCONNECTED);
    
    ESP_LOGI(TAG, "ROS2 manager stopped");
    return ESP_OK;
}

esp_err_t ros2_manager_publish_imu(const ros2_imu_msg_t* imu_data)
{
    if (!ros2_manager_initialized || !ros2_manager_started) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!imu_data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Queue IMU data for publishing
    if (xQueueSend(imu_publish_queue, imu_data, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGW(TAG, "IMU publish queue full, dropping message");
        current_stats.publish_errors++;
        return ESP_ERR_TIMEOUT;
    }
    
    return ESP_OK;
}

bool ros2_manager_is_connected(void)
{
    return (current_status == ROS2_STATUS_CONNECTED || 
            current_status == ROS2_STATUS_PUBLISHING || 
            current_status == ROS2_STATUS_SUBSCRIBING);
}

ros2_status_t ros2_manager_get_status(void)
{
    return current_status;
}

esp_err_t ros2_manager_get_statistics(ros2_statistics_t* stats)
{
    if (!stats) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Update uptime
    current_stats.total_uptime_ms = xTaskGetTickCount() * portTICK_PERIOD_MS - initialization_time;
    
    memcpy(stats, &current_stats, sizeof(ros2_statistics_t));
    return ESP_OK;
}

void ros2_manager_set_connection_callback(ros2_connection_callback_t callback)
{
    connection_callback = callback;
}

void ros2_manager_set_image_callback(ros2_image_callback_t callback)
{
    image_callback = callback;
}

void ros2_manager_set_error_callback(ros2_error_callback_t callback)
{
    error_callback = callback;
}

esp_err_t ros2_manager_bno055_to_imu_msg(const void* quat_ptr, ros2_imu_msg_t* imu_msg)
{
    if (!quat_ptr || !imu_msg) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Assuming quat_ptr points to bno055_quaternion_t structure
    typedef struct {
        float w, x, y, z;
    } bno055_quaternion_t;
    
    const bno055_quaternion_t* quat = (const bno055_quaternion_t*)quat_ptr;
    
    // Clear message
    memset(imu_msg, 0, sizeof(ros2_imu_msg_t));
    
    // Fill header
    imu_msg->seq = sequence_number++;
    imu_msg->timestamp_ns = (uint64_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) * 1000000ULL;
    strcpy(imu_msg->frame_id, "m5atom_imu");
    
    // Fill orientation (quaternion)
    imu_msg->orientation_x = quat->x;
    imu_msg->orientation_y = quat->y;
    imu_msg->orientation_z = quat->z;
    imu_msg->orientation_w = quat->w;
    
    // Set orientation covariance (unknown)
    for (int i = 0; i < 9; i++) {
        imu_msg->orientation_covariance[i] = (i % 4 == 0) ? -1.0f : 0.0f;
    }
    
    // Angular velocity and linear acceleration are not available from BNO055 quaternion only
    // Set covariances to indicate unknown
    for (int i = 0; i < 9; i++) {
        imu_msg->angular_velocity_covariance[i] = (i % 4 == 0) ? -1.0f : 0.0f;
        imu_msg->linear_acceleration_covariance[i] = (i % 4 == 0) ? -1.0f : 0.0f;
    }
    
    return ESP_OK;
}

const char* ros2_manager_status_to_string(ros2_status_t status)
{
    switch (status) {
        case ROS2_STATUS_DISCONNECTED:  return "DISCONNECTED";
        case ROS2_STATUS_CONNECTING:    return "CONNECTING";
        case ROS2_STATUS_CONNECTED:     return "CONNECTED";
        case ROS2_STATUS_PUBLISHING:    return "PUBLISHING";
        case ROS2_STATUS_SUBSCRIBING:   return "SUBSCRIBING";
        case ROS2_STATUS_ERROR:         return "ERROR";
        case ROS2_STATUS_TIMEOUT:       return "TIMEOUT";
        default:                        return "UNKNOWN";
    }
}

// Always enable mock mode functions for testing
void ros2_manager_set_mock_mode(bool enable)
{
    ros2_mock_mode = enable;
    ESP_LOGI(TAG, "Mock mode %s", enable ? "enabled" : "disabled");
}

esp_err_t ros2_manager_mock_receive_image(const ros2_compressed_image_msg_t* image)
{
    if (!ros2_mock_mode || !image) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (image_callback) {
        image_callback(image);
    }
    
    current_stats.messages_received++;
    return ESP_OK;
}

// Mock mode functions
#ifdef ROS2_MANAGER_MOCK_MODE
void ros2_manager_set_mock_mode(bool enable)
{
    ros2_mock_mode = enable;
    ESP_LOGI(TAG, "Mock mode %s", enable ? "enabled" : "disabled");
}

esp_err_t ros2_manager_mock_receive_image(const ros2_compressed_image_msg_t* image)
{
    if (!ros2_mock_mode || !image) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (image_callback) {
        image_callback(image);
    }
    
    current_stats.messages_received++;
    return ESP_OK;
}
#endif

// Internal task implementations
static void publish_task(void *pvParameters)
{
    ESP_LOGI(TAG, "ROS2 publish task started");
    
    ros2_imu_msg_t imu_msg;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000 / current_config.publish_rate_hz);
    
    while (1) {
        // Wait for next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        // Check if we're connected
        if (!ros2_manager_is_connected()) {
            continue;
        }
        
        // Process IMU messages from queue
        while (xQueueReceive(imu_publish_queue, &imu_msg, 0) == pdPASS) {
            notify_status_change(ROS2_STATUS_PUBLISHING);
            
            esp_err_t result;
            if (ros2_mock_mode) {
                result = simulate_ros2_publish(&imu_msg);
            } else {
                // Actual ROS2 publishing would go here
                result = ESP_ERR_NOT_SUPPORTED;
                ESP_LOGW(TAG, "Actual ROS2 publishing not implemented, using simulation");
                result = simulate_ros2_publish(&imu_msg);
            }
            
            if (result == ESP_OK) {
                current_stats.messages_published++;
                last_publish_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            } else {
                current_stats.publish_errors++;
                notify_error(result, "Failed to publish IMU message");
            }
        }
        
        // Update status back to connected if no more messages
        if (ros2_manager_is_connected() && current_status == ROS2_STATUS_PUBLISHING) {
            notify_status_change(ROS2_STATUS_CONNECTED);
        }
    }
}

static void subscribe_task(void *pvParameters)
{
    ESP_LOGI(TAG, "ROS2 subscribe task started");
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100);  // Check every 100ms
    
    while (1) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        // Check if we're connected
        if (!ros2_manager_is_connected()) {
            continue;
        }
        
        notify_status_change(ROS2_STATUS_SUBSCRIBING);
        
        // Simulate image reception in mock mode
        if (ros2_mock_mode) {
            simulate_image_reception();
        } else {
            // Actual ROS2 subscription would go here
            ESP_LOGD(TAG, "Listening for ROS2 messages (not implemented)");
        }
        
        // Update status back to connected
        if (ros2_manager_is_connected() && current_status == ROS2_STATUS_SUBSCRIBING) {
            notify_status_change(ROS2_STATUS_CONNECTED);
        }
    }
}

static void connection_timer_callback(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "Connection timer expired, attempting connection");
    
    esp_err_t result = simulate_ros2_connection();
    if (result == ESP_OK) {
        notify_status_change(ROS2_STATUS_CONNECTED);
        current_stats.successful_connections++;
        ESP_LOGI(TAG, "ROS2 connection established");
    } else {
        notify_status_change(ROS2_STATUS_ERROR);
        notify_error(result, "Connection timeout");
        ESP_LOGE(TAG, "ROS2 connection failed");
    }
}

static void notify_status_change(ros2_status_t new_status)
{
    if (current_status != new_status) {
        ros2_status_t old_status = current_status;
        current_status = new_status;
        
        ESP_LOGD(TAG, "Status change: %s -> %s", 
                ros2_manager_status_to_string(old_status),
                ros2_manager_status_to_string(new_status));
        
        if (connection_callback) {
            connection_callback(new_status);
        }
        
        // Track disconnection events
        if (old_status != ROS2_STATUS_DISCONNECTED && new_status == ROS2_STATUS_DISCONNECTED) {
            current_stats.disconnection_events++;
        }
    }
}

static void notify_error(esp_err_t error, const char* message)
{
    ESP_LOGE(TAG, "ROS2 error: %s (%s)", message, esp_err_to_name(error));
    
    if (error_callback) {
        error_callback(error, message);
    }
}

static esp_err_t simulate_ros2_connection(void)
{
    // Simulate connection delay
    ESP_LOGI(TAG, "Simulating ROS2 connection to node '%s'", current_config.node_name);
    vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second connection time
    
    // Always succeed in simulation
    return ESP_OK;
}

static esp_err_t simulate_ros2_publish(const ros2_imu_msg_t* imu_data)
{
    ESP_LOGD(TAG, "Publishing IMU: seq=%lu, quat=(%.3f,%.3f,%.3f,%.3f)", 
             imu_data->seq,
             imu_data->orientation_w, imu_data->orientation_x,
             imu_data->orientation_y, imu_data->orientation_z);
    
    return ESP_OK;
}

static void simulate_image_reception(void)
{
    static uint32_t sim_counter = 0;
    
    // Simulate receiving an image every few seconds
    if ((sim_counter % 50) == 0) {  // Every ~5 seconds at 100ms intervals
        ros2_compressed_image_msg_t sim_image = {0};
        
        sim_image.seq = sim_counter / 50;
        sim_image.timestamp_ns = (uint64_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) * 1000000ULL;
        strcpy(sim_image.frame_id, "camera");
        strcpy(sim_image.format, "jpeg");
        sim_image.data_size = 1024;  // Simulate 1KB image
        
        ESP_LOGD(TAG, "Simulated image received: seq=%lu, size=%zu", sim_image.seq, sim_image.data_size);
        
        if (image_callback) {
            image_callback(&sim_image);
        }
        
        current_stats.messages_received++;
    }
    
    sim_counter++;
}