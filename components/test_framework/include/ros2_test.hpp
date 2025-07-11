#ifndef ROS2_TEST_HPP
#define ROS2_TEST_HPP

#include "base_test.hpp"
#include "ros2_manager.h"
#include "bno055.h"
#include <string>
#include <vector>

class ROS2Test : public BaseTest {
public:
    ROS2Test();
    virtual ~ROS2Test() = default;

    // Implement base test methods
    esp_err_t setup() override;
    esp_err_t execute() override;
    esp_err_t teardown() override;

    // ROS2-specific methods
    esp_err_t initializeROS2Manager();
    esp_err_t testROS2Connection();
    esp_err_t testIMUPublishing();
    esp_err_t testImageSubscription();
    esp_err_t testCommunicationStability();
    esp_err_t testMessageThroughput();

    // Configuration
    void setNodeName(const std::string& node_name);
    void setIMUTopic(const std::string& topic_name);
    void setImageTopic(const std::string& topic_name);
    void setPublishRate(uint32_t rate_hz) { publish_rate_hz_ = rate_hz; }
    void setConnectionTimeout(uint32_t timeout_ms) { connection_timeout_ms_ = timeout_ms; }
    void setStabilityTestDuration(uint32_t duration_ms) { stability_test_duration_ = duration_ms; }
    void setIMUReadingCount(int count) { imu_reading_count_ = count; }
    void setExpectedImageCount(int count) { expected_image_count_ = count; }

    // BNO055 integration
    void setBNO055Config(const bno055_config_t& config) { bno055_config_ = config; }
    void setEnableBNO055(bool enable) { enable_bno055_ = enable; }

private:
    // Configuration
    ros2_manager_config_t ros2_config_;
    bno055_config_t bno055_config_;
    uint32_t publish_rate_hz_;
    uint32_t connection_timeout_ms_;
    uint32_t stability_test_duration_;
    int imu_reading_count_;
    int expected_image_count_;
    bool enable_bno055_;
    
    // Test state
    bool ros2_manager_initialized_;
    bool bno055_initialized_;
    bool connection_established_;
    
    // Test results and statistics
    uint32_t messages_published_;
    uint32_t messages_received_;
    uint32_t publish_errors_;
    uint32_t receive_errors_;
    uint32_t connection_attempts_;
    uint32_t successful_connections_;
    std::vector<uint64_t> publish_timestamps_;
    std::vector<uint64_t> receive_timestamps_;
    
    // Event tracking
    static ROS2Test* instance_;  // For static callbacks
    static void connectionStatusCallback(ros2_status_t status);
    static void imageReceivedCallback(const ros2_compressed_image_msg_t* image);
    static void errorCallback(esp_err_t error, const char* message);
    
    void handleConnectionStatus(ros2_status_t status);
    void handleImageReceived(const ros2_compressed_image_msg_t* image);
    void handleError(esp_err_t error, const char* message);
    
    // Helper methods
    esp_err_t initializeBNO055();
    esp_err_t waitForConnection();
    esp_err_t publishIMUData();
    esp_err_t validateROS2Statistics();
    esp_err_t calculateThroughputMetrics();
    void resetCounters();
    void logROS2Statistics();
    void logCommunicationMetrics();
    
    // BNO055 integration helpers
    esp_err_t readBNO055AndPublish();
    esp_err_t convertBNO055ToROS2(const bno055_quaternion_t& quat, ros2_imu_msg_t& imu_msg);
    
    // Test validation
    bool isConnectionStable();
    bool isThroughputAcceptable();
    esp_err_t validateMessageIntegrity();
    
    // Mock mode for testing without actual ROS2
    bool mock_mode_;
    void enableMockMode(bool enable) { mock_mode_ = enable; }
    esp_err_t simulateMockCommunication();
};

#endif // ROS2_TEST_HPP