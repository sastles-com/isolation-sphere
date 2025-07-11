#include "base_test.hpp"
#include <cstdarg>
#include <cstring>

const char* BaseTest::TAG = "BaseTest";

BaseTest::BaseTest(const std::string& name, const std::string& description)
    : test_name_(name), test_description_(description)
{
    status_.result = TestResult::NOT_RUN;
    status_.message = "";
    status_.duration_ms = 0;
    status_.start_time = 0;
    status_.error_code = 0;
}

TestResult BaseTest::run()
{
    logInfo("Starting test: %s", test_name_.c_str());
    logInfo("Description: %s", test_description_.c_str());
    
    updateStatus(TestResult::RUNNING, "Test execution started");
    startTimer();
    
    esp_err_t result = ESP_OK;
    
    // Setup phase
    logInfo("=== Setup Phase ===");
    result = setup();
    if (result != ESP_OK) {
        stopTimer();
        updateStatus(TestResult::FAILED, "Setup phase failed");
        logError("Setup failed with error: %s", esp_err_to_name(result));
        return TestResult::FAILED;
    }
    logPass("Setup completed successfully");
    
    // Execute phase
    logInfo("=== Execute Phase ===");
    result = execute();
    if (result != ESP_OK) {
        stopTimer();
        updateStatus(TestResult::FAILED, "Execute phase failed");
        logError("Execute failed with error: %s", esp_err_to_name(result));
        
        // Still run teardown even if execute failed
        logInfo("=== Teardown Phase ===");
        teardown();
        return TestResult::FAILED;
    }
    logPass("Execute completed successfully");
    
    // Teardown phase
    logInfo("=== Teardown Phase ===");
    result = teardown();
    if (result != ESP_OK) {
        stopTimer();
        updateStatus(TestResult::FAILED, "Teardown phase failed");
        logError("Teardown failed with error: %s", esp_err_to_name(result));
        return TestResult::FAILED;
    }
    logPass("Teardown completed successfully");
    
    stopTimer();
    updateStatus(TestResult::PASSED, "Test completed successfully");
    logPass("Test '%s' PASSED in %lu ms", test_name_.c_str(), status_.duration_ms);
    
    return TestResult::PASSED;
}

void BaseTest::addStep(const std::string& name, std::function<esp_err_t()> step, 
                       uint32_t timeout_ms, bool critical)
{
    TestStep test_step;
    test_step.name = name;
    test_step.execute = step;
    test_step.timeout_ms = timeout_ms;
    test_step.critical = critical;
    
    test_steps_.push_back(test_step);
}

esp_err_t BaseTest::runSteps()
{
    for (const auto& step : test_steps_) {
        logInfo("Executing step: %s", step.name.c_str());
        
        esp_err_t result = executeStep(step);
        if (result != ESP_OK) {
            logError("Step '%s' failed: %s", step.name.c_str(), esp_err_to_name(result));
            if (step.critical) {
                return result;
            } else {
                logError("Non-critical step failed, continuing...");
            }
        } else {
            logPass("Step '%s' completed successfully", step.name.c_str());
        }
    }
    return ESP_OK;
}

esp_err_t BaseTest::executeStep(const TestStep& step)
{
    if (!step.execute) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Execute the step
    esp_err_t result = step.execute();
    
    return result;
}

void BaseTest::updateStatus(TestResult result, const std::string& message)
{
    status_.result = result;
    status_.message = message;
    
    if (result != TestResult::RUNNING && result != TestResult::NOT_RUN) {
        // Record error code if failed
        if (result == TestResult::FAILED) {
            status_.error_code = ESP_FAIL;
        } else {
            status_.error_code = ESP_OK;
        }
    }
}

void BaseTest::startTimer()
{
    status_.start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void BaseTest::stopTimer()
{
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    status_.duration_ms = current_time - status_.start_time;
}

void BaseTest::logInfo(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    ESP_LOGI(TAG, "[%s] %s", test_name_.c_str(), buffer);
    
    va_end(args);
}

void BaseTest::logError(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    ESP_LOGE(TAG, "[%s] %s", test_name_.c_str(), buffer);
    
    va_end(args);
}

void BaseTest::logPass(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    ESP_LOGI(TAG, "[%s] ✓ %s", test_name_.c_str(), buffer);
    
    va_end(args);
}

void BaseTest::logFail(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    ESP_LOGE(TAG, "[%s] ✗ %s", test_name_.c_str(), buffer);
    
    va_end(args);
}

std::string BaseTest::resultToString(TestResult result)
{
    switch (result) {
        case TestResult::NOT_RUN:   return "NOT_RUN";
        case TestResult::RUNNING:   return "RUNNING";
        case TestResult::PASSED:    return "PASSED";
        case TestResult::FAILED:    return "FAILED";
        case TestResult::SKIPPED:   return "SKIPPED";
        case TestResult::TIMEOUT:   return "TIMEOUT";
        default:                    return "UNKNOWN";
    }
}

const char* BaseTest::getResultColor(TestResult result)
{
    switch (result) {
        case TestResult::PASSED:    return "\033[32m"; // Green
        case TestResult::FAILED:    return "\033[31m"; // Red
        case TestResult::RUNNING:   return "\033[33m"; // Yellow
        case TestResult::TIMEOUT:   return "\033[35m"; // Magenta
        case TestResult::SKIPPED:   return "\033[36m"; // Cyan
        case TestResult::NOT_RUN:   return "\033[37m"; // White
        default:                    return "\033[0m";  // Reset
    }
}