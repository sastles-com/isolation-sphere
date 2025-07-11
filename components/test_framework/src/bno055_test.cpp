#include "bno055_test.hpp"
#include <cmath>
#include <algorithm>

BNO055Test::BNO055Test()
    : BaseTest("BNO055", "BNO055 IMU sensor quaternion data test"),
      reading_count_(10),
      stability_test_duration_(5000),  // 5 seconds
      quaternion_tolerance_(0.1f),
      sensor_initialized_(false),
      successful_readings_(0),
      failed_readings_(0)
{
    // Default I2C configuration (M5atomS3R GROVE connector)
    sensor_config_.i2c_port = I2C_NUM_0;
    sensor_config_.sda_pin = GPIO_NUM_2;
    sensor_config_.scl_pin = GPIO_NUM_1;
    sensor_config_.i2c_freq = 100000;  // 100kHz
    sensor_config_.i2c_addr = BNO055_I2C_ADDR;
    
    // Initialize quaternion data
    last_quaternion_ = {0.0f, 0.0f, 0.0f, 0.0f};
    last_magnitude_ = 0.0f;
}

esp_err_t BNO055Test::setup()
{
    logInfo("Setting up BNO055 test environment");
    
    // Reset counters
    resetCounters();
    
    // Add test steps
    addStep("Initialize BNO055 sensor", [this]() { return initializeSensor(); });
    addStep("Test sensor communication", [this]() { return testSensorCommunication(); });
    addStep("Test quaternion reading", [this]() { return testQuaternionReading(); });
    addStep("Test sensor calibration", [this]() { return testSensorCalibration(); });
    addStep("Test data consistency", [this]() { return testDataConsistency(); });
    addStep("Perform stability test", [this]() { return performStabilityTest(); });
    
    logPass("BNO055 test setup completed");
    return ESP_OK;
}

esp_err_t BNO055Test::execute()
{
    logInfo("Executing BNO055 test steps");
    
    // Log configuration
    logInfo("BNO055 Configuration:");
    logInfo("  I2C Port: %d", sensor_config_.i2c_port);
    logInfo("  SDA Pin: %d", sensor_config_.sda_pin);
    logInfo("  SCL Pin: %d", sensor_config_.scl_pin);
    logInfo("  I2C Frequency: %lu Hz", sensor_config_.i2c_freq);
    logInfo("  I2C Address: 0x%02X", sensor_config_.i2c_addr);
    
    // Run all test steps
    esp_err_t ret = runSteps();
    if (ret != ESP_OK) {
        logError("BNO055 test execution failed");
        return ret;
    }
    
    // Log final statistics
    logInfo("Final Test Statistics:");
    logInfo("  Successful readings: %lu", successful_readings_);
    logInfo("  Failed readings: %lu", failed_readings_);
    float success_rate = (float)successful_readings_ / (successful_readings_ + failed_readings_) * 100.0f;
    logInfo("  Success rate: %.1f%%", success_rate);
    
    logPass("BNO055 test execution completed successfully");
    return ESP_OK;
}

esp_err_t BNO055Test::teardown()
{
    logInfo("Cleaning up BNO055 test");
    
    // Clear quaternion history
    quaternion_history_.clear();
    
    // Note: bno055_deinit() would be called here if the driver supported it
    sensor_initialized_ = false;
    
    logPass("BNO055 test cleanup completed");
    return ESP_OK;
}

esp_err_t BNO055Test::initializeSensor()
{
    logInfo("Initializing BNO055 sensor");
    
    // Initialize BNO055
    esp_err_t ret = bno055_init(&sensor_config_);
    TEST_ASSERT_OK(ret);
    
    sensor_initialized_ = true;
    
    // Wait for sensor stabilization
    ret = waitForSensorStabilization();
    TEST_ASSERT_OK(ret);
    
    logPass("BNO055 sensor initialized successfully");
    return ESP_OK;
}

esp_err_t BNO055Test::testSensorCommunication()
{
    logInfo("Testing BNO055 sensor communication");
    
    TEST_ASSERT(sensor_initialized_, "Sensor must be initialized first");
    
    // Check sensor ID (if available in driver)
    esp_err_t ret = checkSensorID();
    if (ret == ESP_ERR_NOT_SUPPORTED) {
        logInfo("Sensor ID check not supported by driver, skipping");
    } else {
        TEST_ASSERT_OK(ret);
    }
    
    // Test basic quaternion reading to verify communication
    bno055_quaternion_t test_quat;
    ret = bno055_get_quaternion(&test_quat);
    TEST_ASSERT_OK(ret);
    
    // Validate the quaternion data structure
    TEST_ASSERT(isQuaternionValid(test_quat), "Invalid quaternion data received");
    
    logPass("BNO055 sensor communication test passed");
    return ESP_OK;
}

esp_err_t BNO055Test::testQuaternionReading()
{
    logInfo("Testing BNO055 quaternion reading (%d readings)", reading_count_);
    
    TEST_ASSERT(sensor_initialized_, "Sensor must be initialized first");
    
    for (int i = 0; i < reading_count_; i++) {
        bno055_quaternion_t quat;
        esp_err_t ret = ESP_FAIL;
        
        // Retry mechanism for I2C timeouts
        for (int retry = 0; retry < 3; retry++) {
            ret = bno055_get_quaternion(&quat);
            if (ret == ESP_OK) {
                break;
            } else if (retry < 2) {
                logInfo("Retrying quaternion read %d (attempt %d/3)", i + 1, retry + 2);
                vTaskDelay(pdMS_TO_TICKS(50));  // Short delay before retry
            }
        }
        
        if (ret == ESP_OK) {
            successful_readings_++;
            
            // Validate quaternion
            esp_err_t val_ret = validateQuaternion(quat);
            if (val_ret == ESP_OK) {
                last_quaternion_ = quat;
                last_magnitude_ = calculateMagnitude(quat);
                quaternion_history_.push_back(quat);
                
                logQuaternionData(quat, i + 1);
            } else {
                logError("Invalid quaternion data at reading %d", i + 1);
                failed_readings_++;
            }
        } else {
            failed_readings_++;
            logError("Failed to read quaternion at reading %d after 3 attempts: %s", i + 1, esp_err_to_name(ret));
        }
        
        // Small delay between readings
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Check that we got at least some successful readings
    TEST_ASSERT(successful_readings_ > 0, "No successful quaternion readings");
    
    float success_rate = (float)successful_readings_ / reading_count_ * 100.0f;
    logPass("Quaternion reading test completed: %.1f%% success rate", success_rate);
    
    return ESP_OK;
}

esp_err_t BNO055Test::testSensorCalibration()
{
    logInfo("Testing BNO055 sensor calibration status");
    
    TEST_ASSERT(sensor_initialized_, "Sensor must be initialized first");
    
    // Note: Calibration status reading would require additional driver support
    // For now, we'll check if quaternion magnitude is reasonable (close to 1.0)
    
    if (quaternion_history_.empty()) {
        logInfo("No quaternion history available, performing single reading");
        bno055_quaternion_t quat;
        esp_err_t ret = bno055_get_quaternion(&quat);
        TEST_ASSERT_OK(ret);
        last_magnitude_ = calculateMagnitude(quat);
    }
    
    // Check if magnitude is reasonable (should be close to 1.0 for normalized quaternion)
    float expected_magnitude = 1.0f;
    float magnitude_error = fabs(last_magnitude_ - expected_magnitude);
    
    logInfo("Quaternion magnitude: %.4f (expected: %.4f, error: %.4f)", 
            last_magnitude_, expected_magnitude, magnitude_error);
    
    TEST_ASSERT(magnitude_error < quaternion_tolerance_, 
                "Quaternion magnitude outside acceptable range");
    
    logPass("Sensor calibration test passed (magnitude within tolerance)");
    return ESP_OK;
}

esp_err_t BNO055Test::testDataConsistency()
{
    logInfo("Testing BNO055 data consistency");
    
    TEST_ASSERT(sensor_initialized_, "Sensor must be initialized first");
    TEST_ASSERT(quaternion_history_.size() >= 2, "Need at least 2 readings for consistency test");
    
    // Check for reasonable variation in quaternion data
    // (data should change but not wildly)
    
    float max_change = 0.0f;
    float total_change = 0.0f;
    
    for (size_t i = 1; i < quaternion_history_.size(); i++) {
        const auto& prev = quaternion_history_[i-1];
        const auto& curr = quaternion_history_[i];
        
        float change = sqrt(
            (curr.w - prev.w) * (curr.w - prev.w) +
            (curr.x - prev.x) * (curr.x - prev.x) +
            (curr.y - prev.y) * (curr.y - prev.y) +
            (curr.z - prev.z) * (curr.z - prev.z)
        );
        
        max_change = std::max(max_change, change);
        total_change += change;
    }
    
    float avg_change = total_change / (quaternion_history_.size() - 1);
    
    logInfo("Data consistency analysis:");
    logInfo("  Maximum change between readings: %.4f", max_change);
    logInfo("  Average change between readings: %.4f", avg_change);
    
    // Check for reasonable data variation or stability
    // In stable conditions, data can be very consistent, which is actually good
    if (max_change < 0.0001f) {
        logInfo("Sensor data is very stable (max change: %.6f)", max_change);
        // This is acceptable for a stationary sensor
    } else {
        logInfo("Sensor data shows variation (max change: %.4f)", max_change);
    }
    
    // Data should not change too wildly between readings (indicating malfunction)
    TEST_ASSERT(max_change < 2.0f, "Data changes too dramatically (possible sensor malfunction)");
    
    logPass("Data consistency test passed");
    return ESP_OK;
}

esp_err_t BNO055Test::performStabilityTest()
{
    logInfo("Performing BNO055 stability test (%d ms)", stability_test_duration_);
    
    TEST_ASSERT(sensor_initialized_, "Sensor must be initialized first");
    
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t readings_in_stability_test = 0;
    uint32_t errors_in_stability_test = 0;
    
    while ((xTaskGetTickCount() * portTICK_PERIOD_MS - start_time) < stability_test_duration_) {
        bno055_quaternion_t quat;
        esp_err_t ret = bno055_get_quaternion(&quat);
        
        readings_in_stability_test++;
        
        if (ret == ESP_OK && validateQuaternion(quat) == ESP_OK) {
            // Success - no action needed
        } else {
            errors_in_stability_test++;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  // 20Hz reading rate
    }
    
    float stability_success_rate = (float)(readings_in_stability_test - errors_in_stability_test) / 
                                   readings_in_stability_test * 100.0f;
    
    logInfo("Stability test results:");
    logInfo("  Total readings: %lu", readings_in_stability_test);
    logInfo("  Errors: %lu", errors_in_stability_test);
    logInfo("  Success rate: %.1f%%", stability_success_rate);
    
    // Require at least 80% success rate for stability (adjusted for I2C hardware limitations)
    TEST_ASSERT(stability_success_rate >= 80.0f, "Stability test failed: too many errors");
    
    logPass("Stability test passed");
    return ESP_OK;
}

void BNO055Test::setI2CConfig(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t freq)
{
    sensor_config_.i2c_port = port;
    sensor_config_.sda_pin = sda;
    sensor_config_.scl_pin = scl;
    sensor_config_.i2c_freq = freq;
}

esp_err_t BNO055Test::validateQuaternion(const bno055_quaternion_t& quat)
{
    // Check for NaN values
    if (std::isnan(quat.w) || std::isnan(quat.x) || std::isnan(quat.y) || std::isnan(quat.z)) {
        return ESP_FAIL;
    }
    
    // Check for infinite values  
    if (std::isinf(quat.w) || std::isinf(quat.x) || std::isinf(quat.y) || std::isinf(quat.z)) {
        return ESP_FAIL;
    }
    
    // Check magnitude is reasonable
    float magnitude = calculateMagnitude(quat);
    if (magnitude < 0.1f || magnitude > 2.0f) {
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

float BNO055Test::calculateMagnitude(const bno055_quaternion_t& quat)
{
    return sqrt(quat.w*quat.w + quat.x*quat.x + quat.y*quat.y + quat.z*quat.z);
}

esp_err_t BNO055Test::checkSensorID()
{
    // This would require driver support for reading sensor ID
    // Return not supported for now
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t BNO055Test::waitForSensorStabilization()
{
    logInfo("Waiting for sensor stabilization (2 seconds)");
    vTaskDelay(pdMS_TO_TICKS(2000));
    return ESP_OK;
}

bool BNO055Test::isQuaternionValid(const bno055_quaternion_t& quat)
{
    return validateQuaternion(quat) == ESP_OK;
}

void BNO055Test::logQuaternionData(const bno055_quaternion_t& quat, int reading_num)
{
    float magnitude = calculateMagnitude(quat);
    
    if (reading_num >= 0) {
        logInfo("Quaternion #%d: W=%+.4f, X=%+.4f, Y=%+.4f, Z=%+.4f, |q|=%.4f", 
                reading_num, quat.w, quat.x, quat.y, quat.z, magnitude);
    } else {
        logInfo("Quaternion: W=%+.4f, X=%+.4f, Y=%+.4f, Z=%+.4f, |q|=%.4f", 
                quat.w, quat.x, quat.y, quat.z, magnitude);
    }
}

void BNO055Test::resetCounters()
{
    successful_readings_ = 0;
    failed_readings_ = 0;
    quaternion_history_.clear();
}