#ifndef BASE_TEST_HPP
#define BASE_TEST_HPP

#include <string>
#include <vector>
#include <functional>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Test result enum
enum class TestResult {
    NOT_RUN = 0,
    RUNNING,
    PASSED,
    FAILED,
    SKIPPED,
    TIMEOUT
};

// Test status structure
struct TestStatus {
    TestResult result = TestResult::NOT_RUN;
    std::string message;
    uint32_t duration_ms = 0;
    uint32_t start_time = 0;
    uint32_t error_code = 0;
};

// Test step structure
struct TestStep {
    std::string name;
    std::function<esp_err_t()> execute;
    uint32_t timeout_ms = 5000;
    bool critical = true;  // If true, failure stops the test
};

// Base test class
class BaseTest {
public:
    BaseTest(const std::string& name, const std::string& description);
    virtual ~BaseTest() = default;

    // Core test methods
    virtual esp_err_t setup() = 0;
    virtual esp_err_t execute() = 0;
    virtual esp_err_t teardown() = 0;

    // Test runner
    TestResult run();
    
    // Getters
    const std::string& getName() const { return test_name_; }
    const std::string& getDescription() const { return test_description_; }
    const TestStatus& getStatus() const { return status_; }
    
    // Test utilities
    void addStep(const std::string& name, std::function<esp_err_t()> step, 
                 uint32_t timeout_ms = 5000, bool critical = true);
    void logInfo(const char* format, ...);
    void logError(const char* format, ...);
    void logPass(const char* format, ...);
    void logFail(const char* format, ...);
    
    // Static test utilities
    static std::string resultToString(TestResult result);
    static const char* getResultColor(TestResult result);

protected:
    std::string test_name_;
    std::string test_description_;
    TestStatus status_;
    std::vector<TestStep> test_steps_;
    
    // Protected utilities
    esp_err_t runSteps();
    esp_err_t executeStep(const TestStep& step);
    void updateStatus(TestResult result, const std::string& message = "");
    void startTimer();
    void stopTimer();

private:
    static const char* TAG;
};

// Test result helper macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            logFail("Assertion failed: %s", message); \
            return ESP_FAIL; \
        } \
    } while(0)

#define TEST_ASSERT_OK(esp_err) \
    do { \
        esp_err_t _err = (esp_err); \
        if (_err != ESP_OK) { \
            logFail("ESP error: %s (%d)", esp_err_to_name(_err), _err); \
            return _err; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    TEST_ASSERT((ptr) != nullptr, message)

#define TEST_ASSERT_EQUAL(expected, actual, message) \
    TEST_ASSERT((expected) == (actual), message)

#define TEST_ASSERT_RANGE(value, min_val, max_val, message) \
    TEST_ASSERT((value) >= (min_val) && (value) <= (max_val), message)

#endif // BASE_TEST_HPP