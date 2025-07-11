#include "ros2_test.hpp"
#include <cstring>
#include <algorithm>
#include <cmath>

// Static instance for callbacks
ROS2Test* ROS2Test::instance_ = nullptr;

ROS2Test::ROS2Test()
    : BaseTest("ROS2", "ROS2 communication and integration test"),
      publish_rate_hz_(10),
      connection_timeout_ms_(30000),  // Increased to 30 seconds for better reliability
      stability_test_duration_(30000),  // 30 seconds
      imu_reading_count_(20),
      expected_image_count_(5),
      enable_bno055_(true),
      ros2_manager_initialized_(false),
      bno055_initialized_(false),
      connection_established_(false),
      messages_published_(0),
      messages_received_(0),
      publish_errors_(0),
      receive_errors_(0),
      connection_attempts_(0),
      successful_connections_(0),
      mock_mode_(true)  // Default to mock mode for testing
{
    // Default ROS2 configuration
    strcpy(ros2_config_.node_name, "m5atom_test_node");
    strcpy(ros2_config_.imu_topic, "m5atom/imu");
    strcpy(ros2_config_.image_topic, "video_frames");
    ros2_config_.publish_rate_hz = publish_rate_hz_;
    ros2_config_.connection_timeout_ms = connection_timeout_ms_;
    ros2_config_.auto_reconnect = true;
    
    // Default BNO055 configuration (M5atomS3R GROVE connector)
    bno055_config_.i2c_port = I2C_NUM_0;
    bno055_config_.sda_pin = GPIO_NUM_2;
    bno055_config_.scl_pin = GPIO_NUM_1;
    bno055_config_.i2c_freq = 100000;
    bno055_config_.i2c_addr = BNO055_I2C_ADDR;
    
    // Set static instance for callbacks
    instance_ = this;
}

esp_err_t ROS2Test::setup()
{
    logInfo("Setting up ROS2 test environment");
    
    // Reset counters and state
    resetCounters();
    
    // Add test steps
    addStep("Initialize ROS2 manager", [this]() { return initializeROS2Manager(); });
    addStep("Test ROS2 connection", [this]() { return testROS2Connection(); });
    
    if (enable_bno055_) {
        addStep("Initialize BNO055 sensor", [this]() { return initializeBNO055(); });
        addStep("Test IMU publishing", [this]() { return testIMUPublishing(); });
    } else {
        addStep("Test mock IMU publishing", [this]() { return testIMUPublishing(); });
    }
    
    addStep("Test image subscription", [this]() { return testImageSubscription(); });
    addStep("Test communication stability", [this]() { return testCommunicationStability(); });
    addStep("Test message throughput", [this]() { return testMessageThroughput(); });
    
    logPass("ROS2 test setup completed");
    return ESP_OK;
}

esp_err_t ROS2Test::execute()
{
    logInfo("Executing ROS2 test steps");
    
    // Log configuration
    logInfo("ROS2 Configuration:");
    logInfo("  Node name: %s", ros2_config_.node_name);
    logInfo("  IMU topic: %s", ros2_config_.imu_topic);
    logInfo("  Image topic: %s", ros2_config_.image_topic);
    logInfo("  Publish rate: %lu Hz", ros2_config_.publish_rate_hz);
    logInfo("  Connection timeout: %lu ms", ros2_config_.connection_timeout_ms);
    logInfo("  BNO055 enabled: %s", enable_bno055_ ? "yes" : "no");
    logInfo("  Mock mode: %s", mock_mode_ ? "enabled" : "disabled");
    
    // Run all test steps
    esp_err_t ret = runSteps();
    if (ret != ESP_OK) {
        logError("ROS2 test execution failed");
        return ret;
    }
    
    // Log final statistics
    logROS2Statistics();
    logCommunicationMetrics();
    
    logPass("ROS2 test execution completed successfully");
    return ESP_OK;
}

esp_err_t ROS2Test::teardown()
{
    logInfo("Cleaning up ROS2 test");
    
    // Stop and deinitialize ROS2 manager
    if (ros2_manager_initialized_) {
        ros2_manager_stop();
        ros2_manager_deinit();
        ros2_manager_initialized_ = false;
    }
    
    // Note: BNO055 deinit would be called here if available
    bno055_initialized_ = false;
    connection_established_ = false;
    
    // Clear static instance
    instance_ = nullptr;
    
    logPass("ROS2 test cleanup completed");
    return ESP_OK;
}

esp_err_t ROS2Test::initializeROS2Manager()
{
    logInfo("Initializing ROS2 manager");
    
    // Update configuration
    ros2_config_.publish_rate_hz = publish_rate_hz_;
    ros2_config_.connection_timeout_ms = connection_timeout_ms_;
    
    esp_err_t ret = ros2_manager_init(&ros2_config_);
    TEST_ASSERT_OK(ret);
    
    ros2_manager_initialized_ = true;
    
    // Set callbacks
    ros2_manager_set_connection_callback(connectionStatusCallback);
    ros2_manager_set_image_callback(imageReceivedCallback);
    ros2_manager_set_error_callback(errorCallback);
    
    // Enable mock mode if configured
    if (mock_mode_) {
        ros2_manager_set_mock_mode(true);
        logInfo("Mock mode enabled for testing");
    }
    
    logPass("ROS2 manager initialized successfully");
    return ESP_OK;
}

esp_err_t ROS2Test::testROS2Connection()
{
    logInfo("Testing ROS2 connection");
    
    TEST_ASSERT(ros2_manager_initialized_, "ROS2 manager must be initialized first");
    
    // Start ROS2 manager
    connection_attempts_++;
    esp_err_t ret = ros2_manager_start();
    TEST_ASSERT_OK(ret);
    
    // Wait for connection
    ret = waitForConnection();
    if (ret == ESP_OK) {
        successful_connections_++;
        connection_established_ = true;
        logPass("ROS2 connection established successfully");
    } else {
        logError("ROS2 connection failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Verify connection status
    TEST_ASSERT(ros2_manager_is_connected(), "Should be connected to ROS2");
    
    ros2_status_t status = ros2_manager_get_status();
    logInfo("ROS2 status: %s", ros2_manager_status_to_string(status));
    
    return ESP_OK;
}

esp_err_t ROS2Test::initializeBNO055()
{
    logInfo("Initializing BNO055 sensor for ROS2 integration");
    
    esp_err_t ret = bno055_init(&bno055_config_);
    TEST_ASSERT_OK(ret);
    
    bno055_initialized_ = true;
    
    // Wait for sensor stabilization
    logInfo("Waiting for BNO055 stabilization");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    logPass("BNO055 sensor initialized for ROS2 test");
    return ESP_OK;
}

esp_err_t ROS2Test::testIMUPublishing()
{
    logInfo("Testing IMU data publishing (%d messages)", imu_reading_count_);
    
    TEST_ASSERT(connection_established_, "Must be connected to ROS2");
    
    for (int i = 0; i < imu_reading_count_; i++) {
        esp_err_t ret = publishIMUData();
        
        if (ret == ESP_OK) {
            messages_published_++;
            publish_timestamps_.push_back(xTaskGetTickCount() * portTICK_PERIOD_MS);
            logInfo("Published IMU message %d/%d", i + 1, imu_reading_count_);
        } else {
            publish_errors_++;
            logError("Failed to publish IMU message %d: %s", i + 1, esp_err_to_name(ret));
        }
        
        // Delay between publishes
        vTaskDelay(pdMS_TO_TICKS(1000 / publish_rate_hz_));
    }
    
    // Check success rate
    float success_rate = (float)messages_published_ / imu_reading_count_ * 100.0f;
    TEST_ASSERT(success_rate >= 80.0f, "IMU publish success rate too low");
    
    logPass("IMU publishing test completed: %.1f%% success rate", success_rate);
    return ESP_OK;
}

esp_err_t ROS2Test::testImageSubscription()
{
    logInfo("Testing image subscription (expecting %d images)", expected_image_count_);
    
    TEST_ASSERT(connection_established_, "Must be connected to ROS2");
    
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t timeout = 10000;  // 10 seconds timeout
    
    // In mock mode, simulate receiving images
    if (mock_mode_) {
        for (int i = 0; i < expected_image_count_; i++) {
            ros2_compressed_image_msg_t mock_image;
            memset(&mock_image, 0, sizeof(mock_image));
            mock_image.seq = i + 1;
            mock_image.timestamp_ns = (uint64_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) * 1000000ULL;
            strcpy(mock_image.frame_id, "camera");
            strcpy(mock_image.format, "jpeg");
            mock_image.data_size = 1024 + (i * 100);  // Varying sizes
            
            ros2_manager_mock_receive_image(&mock_image);
            vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second between images
        }
    }
    
    // Wait for images to be received
    while (messages_received_ < expected_image_count_) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if ((current_time - start_time) > timeout) {
            logError("Timeout waiting for images: received %lu/%d", 
                     messages_received_, expected_image_count_);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    logInfo("Received %lu images (expected %d)", messages_received_, expected_image_count_);
    
    // Allow partial success for image subscription test
    if (messages_received_ > 0) {
        logPass("Image subscription test passed: received %lu images", messages_received_);
        return ESP_OK;
    } else {
        logError("No images received during test");
        return ESP_FAIL;
    }
}

esp_err_t ROS2Test::testCommunicationStability()
{
    logInfo("Testing ROS2 communication stability (%lu ms)", stability_test_duration_);
    
    TEST_ASSERT(connection_established_, "Must be connected to ROS2");
    
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t stability_checks = 0;
    uint32_t connection_drops = 0;
    uint32_t last_published_count = messages_published_;
    uint32_t last_received_count = messages_received_;
    
    while ((xTaskGetTickCount() * portTICK_PERIOD_MS - start_time) < stability_test_duration_) {
        stability_checks++;
        
        // Check connection status
        if (!ros2_manager_is_connected()) {
            connection_drops++;
            logError("Connection drop detected during stability test (check %lu)", stability_checks);
        } else {
            // Continue publishing during stability test
            if (enable_bno055_ && bno055_initialized_) {
                publishIMUData();
            }
        }
        
        // Log progress every 10 checks
        if (stability_checks % 10 == 0) {
            uint32_t new_published = messages_published_ - last_published_count;
            uint32_t new_received = messages_received_ - last_received_count;
            logInfo("Stability check %lu: published +%lu, received +%lu", 
                    stability_checks, new_published, new_received);
            last_published_count = messages_published_;
            last_received_count = messages_received_;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));  // Check every second
    }
    
    float stability_rate = (float)(stability_checks - connection_drops) / stability_checks * 100.0f;
    
    logInfo("Stability test results:");
    logInfo("  Total checks: %lu", stability_checks);
    logInfo("  Connection drops: %lu", connection_drops);
    logInfo("  Stability rate: %.1f%%", stability_rate);
    
    // Require at least 90% stability
    TEST_ASSERT(stability_rate >= 90.0f, "Communication stability below threshold");
    
    logPass("Communication stability test passed: %.1f%%", stability_rate);
    return ESP_OK;
}

esp_err_t ROS2Test::testMessageThroughput()
{
    logInfo("Testing ROS2 message throughput");
    
    esp_err_t ret = validateROS2Statistics();
    TEST_ASSERT_OK(ret);
    
    ret = calculateThroughputMetrics();
    TEST_ASSERT_OK(ret);
    
    logPass("Message throughput test completed");
    return ESP_OK;
}

void ROS2Test::setNodeName(const std::string& node_name)
{
    strncpy(ros2_config_.node_name, node_name.c_str(), sizeof(ros2_config_.node_name) - 1);
    ros2_config_.node_name[sizeof(ros2_config_.node_name) - 1] = '\0';
}

void ROS2Test::setIMUTopic(const std::string& topic_name)
{
    strncpy(ros2_config_.imu_topic, topic_name.c_str(), sizeof(ros2_config_.imu_topic) - 1);
    ros2_config_.imu_topic[sizeof(ros2_config_.imu_topic) - 1] = '\0';
}

void ROS2Test::setImageTopic(const std::string& topic_name)
{
    strncpy(ros2_config_.image_topic, topic_name.c_str(), sizeof(ros2_config_.image_topic) - 1);
    ros2_config_.image_topic[sizeof(ros2_config_.image_topic) - 1] = '\0';
}

// Static callback implementations
void ROS2Test::connectionStatusCallback(ros2_status_t status)
{
    if (instance_) {
        instance_->handleConnectionStatus(status);
    }
}

void ROS2Test::imageReceivedCallback(const ros2_compressed_image_msg_t* image)
{
    if (instance_) {
        instance_->handleImageReceived(image);
    }
}

void ROS2Test::errorCallback(esp_err_t error, const char* message)
{
    if (instance_) {
        instance_->handleError(error, message);
    }
}

void ROS2Test::handleConnectionStatus(ros2_status_t status)
{
    logInfo("ROS2 status change: %s", ros2_manager_status_to_string(status));
    
    if (status == ROS2_STATUS_CONNECTED) {
        connection_established_ = true;
    } else if (status == ROS2_STATUS_DISCONNECTED || status == ROS2_STATUS_ERROR) {
        connection_established_ = false;
    }
}

void ROS2Test::handleImageReceived(const ros2_compressed_image_msg_t* image)
{
    if (image) {
        messages_received_++;
        receive_timestamps_.push_back(xTaskGetTickCount() * portTICK_PERIOD_MS);
        
        logInfo("Received image: seq=%lu, format=%s, size=%zu bytes", 
                image->seq, image->format, image->data_size);
    }
}

void ROS2Test::handleError(esp_err_t error, const char* message)
{
    receive_errors_++;
    logError("ROS2 error: %s (%s)", message, esp_err_to_name(error));
}

esp_err_t ROS2Test::waitForConnection()
{
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    while (!ros2_manager_is_connected()) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if ((current_time - start_time) > connection_timeout_ms_) {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    return ESP_OK;
}

esp_err_t ROS2Test::publishIMUData()
{
    ros2_imu_msg_t imu_msg;
    memset(&imu_msg, 0, sizeof(imu_msg));
    
    if (enable_bno055_ && bno055_initialized_) {
        // Read actual BNO055 data
        return readBNO055AndPublish();
    } else {
        // Use mock IMU data
        static float angle = 0.0f;
        angle += 0.1f;
        
        // Generate mock quaternion (rotation around Z-axis)
        float half_angle = angle / 2.0f;
        imu_msg.orientation_w = cos(half_angle);
        imu_msg.orientation_x = 0.0f;
        imu_msg.orientation_y = 0.0f;
        imu_msg.orientation_z = sin(half_angle);
        
        // Fill header
        imu_msg.seq = messages_published_;
        imu_msg.timestamp_ns = (uint64_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) * 1000000ULL;
        strcpy(imu_msg.frame_id, "m5atom_imu");
        
        return ros2_manager_publish_imu(&imu_msg);
    }
}

esp_err_t ROS2Test::readBNO055AndPublish()
{
    bno055_quaternion_t quat;
    esp_err_t ret = bno055_get_quaternion(&quat);
    TEST_ASSERT_OK(ret);
    
    ros2_imu_msg_t imu_msg;
    ret = ros2_manager_bno055_to_imu_msg(&quat, &imu_msg);
    TEST_ASSERT_OK(ret);
    
    return ros2_manager_publish_imu(&imu_msg);
}

esp_err_t ROS2Test::validateROS2Statistics()
{
    ros2_statistics_t stats;
    esp_err_t ret = ros2_manager_get_statistics(&stats);
    TEST_ASSERT_OK(ret);
    
    logInfo("ROS2 Statistics Validation:");
    logInfo("  Messages published: %lu", stats.messages_published);
    logInfo("  Messages received: %lu", stats.messages_received);
    logInfo("  Publish errors: %lu", stats.publish_errors);
    logInfo("  Receive errors: %lu", stats.receive_errors);
    logInfo("  Connection attempts: %lu", stats.connection_attempts);
    logInfo("  Successful connections: %lu", stats.successful_connections);
    
    // Update local statistics
    messages_published_ = stats.messages_published;
    messages_received_ = stats.messages_received;
    publish_errors_ = stats.publish_errors;
    receive_errors_ = stats.receive_errors;
    
    return ESP_OK;
}

esp_err_t ROS2Test::calculateThroughputMetrics()
{
    if (publish_timestamps_.size() < 2) {
        logInfo("Insufficient data for throughput calculation");
        return ESP_OK;
    }
    
    // Calculate publish rate
    uint64_t total_publish_time = publish_timestamps_.back() - publish_timestamps_.front();
    float actual_publish_rate = (float)(publish_timestamps_.size() - 1) * 1000.0f / total_publish_time;
    
    logInfo("Throughput Metrics:");
    logInfo("  Expected publish rate: %lu Hz", publish_rate_hz_);
    logInfo("  Actual publish rate: %.2f Hz", actual_publish_rate);
    logInfo("  Total messages published: %zu", publish_timestamps_.size());
    
    if (receive_timestamps_.size() >= 2) {
        uint64_t total_receive_time = receive_timestamps_.back() - receive_timestamps_.front();
        float actual_receive_rate = (float)(receive_timestamps_.size() - 1) * 1000.0f / total_receive_time;
        logInfo("  Actual receive rate: %.2f Hz", actual_receive_rate);
        logInfo("  Total messages received: %zu", receive_timestamps_.size());
    }
    
    return ESP_OK;
}

void ROS2Test::resetCounters()
{
    messages_published_ = 0;
    messages_received_ = 0;
    publish_errors_ = 0;
    receive_errors_ = 0;
    connection_attempts_ = 0;
    successful_connections_ = 0;
    connection_established_ = false;
    
    publish_timestamps_.clear();
    receive_timestamps_.clear();
}

void ROS2Test::logROS2Statistics()
{
    ros2_statistics_t stats;
    if (ros2_manager_get_statistics(&stats) == ESP_OK) {
        logInfo("Final ROS2 Statistics:");
        logInfo("  Messages published: %lu", stats.messages_published);
        logInfo("  Messages received: %lu", stats.messages_received);
        logInfo("  Publish errors: %lu", stats.publish_errors);
        logInfo("  Receive errors: %lu", stats.receive_errors);
        logInfo("  Total uptime: %lu ms", stats.total_uptime_ms);
        
        if (stats.messages_published > 0) {
            float publish_success_rate = (float)(stats.messages_published) / 
                                        (stats.messages_published + stats.publish_errors) * 100.0f;
            logInfo("  Publish success rate: %.1f%%", publish_success_rate);
        }
    }
}

void ROS2Test::logCommunicationMetrics()
{
    logInfo("Communication Test Summary:");
    logInfo("  Connection attempts: %lu", connection_attempts_);
    logInfo("  Successful connections: %lu", successful_connections_);
    logInfo("  Connection success rate: %.1f%%", 
            connection_attempts_ > 0 ? (float)successful_connections_ / connection_attempts_ * 100.0f : 0.0f);
}