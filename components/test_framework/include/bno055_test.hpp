#ifndef BNO055_TEST_HPP
#define BNO055_TEST_HPP

#include "base_test.hpp"
#include "bno055.h"
#include <vector>

class BNO055Test : public BaseTest {
public:
    BNO055Test();
    virtual ~BNO055Test() = default;

    // Implement base test methods
    esp_err_t setup() override;
    esp_err_t execute() override;
    esp_err_t teardown() override;

    // BNO055-specific methods
    esp_err_t initializeSensor();
    esp_err_t testSensorCommunication();
    esp_err_t testQuaternionReading();
    esp_err_t testSensorCalibration();
    esp_err_t testDataConsistency();
    esp_err_t performStabilityTest();

    // Configuration
    void setI2CConfig(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t freq = 100000);
    void setReadingCount(int count) { reading_count_ = count; }
    void setStabilityTestDuration(int duration_ms) { stability_test_duration_ = duration_ms; }
    void setQuaternionTolerance(float tolerance) { quaternion_tolerance_ = tolerance; }

private:
    // Configuration
    bno055_config_t sensor_config_;
    int reading_count_;
    int stability_test_duration_;
    float quaternion_tolerance_;
    
    // Test state
    bool sensor_initialized_;
    std::vector<bno055_quaternion_t> quaternion_history_;
    
    // Test results
    bno055_quaternion_t last_quaternion_;
    float last_magnitude_;
    uint32_t successful_readings_;
    uint32_t failed_readings_;
    
    // Helper methods
    esp_err_t validateQuaternion(const bno055_quaternion_t& quat);
    float calculateMagnitude(const bno055_quaternion_t& quat);
    esp_err_t checkSensorID();
    esp_err_t waitForSensorStabilization();
    bool isQuaternionValid(const bno055_quaternion_t& quat);
    void logQuaternionData(const bno055_quaternion_t& quat, int reading_num = -1);
    void resetCounters();
};

#endif // BNO055_TEST_HPP