#include "test_manager.hpp"
#include "esp_log.h"
#include <algorithm>
#include <cstring>

const char* TestManager::TAG = "TestManager";

TestManager::TestManager()
    : stop_on_first_failure_(false),
      parallel_execution_(false),
      test_timeout_ms_(300000),  // 5 minutes default
      overall_result_(TestResult::NOT_RUN),
      execution_start_time_(0),
      execution_end_time_(0)
{
    ESP_LOGI(TAG, "Test Manager initialized");
}

void TestManager::addTest(std::unique_ptr<BaseTest> test)
{
    if (test) {
        tests_.push_back(std::shared_ptr<BaseTest>(test.release()));
        ESP_LOGI(TAG, "Added test: %s", tests_.back()->getName().c_str());
    }
}

void TestManager::addTest(std::shared_ptr<BaseTest> test)
{
    if (test) {
        tests_.push_back(test);
        ESP_LOGI(TAG, "Added test: %s", test->getName().c_str());
    }
}

bool TestManager::runAllTests()
{
    ESP_LOGI(TAG, "Starting test execution for %zu tests", tests_.size());
    printSeparator('=', 80);
    ESP_LOGI(TAG, "                    TEST EXECUTION STARTED                     ");
    printSeparator('=', 80);
    
    if (tests_.empty()) {
        ESP_LOGW(TAG, "No tests to execute");
        overall_result_ = TestResult::SKIPPED;
        return true;
    }
    
    execution_start_time_ = xTaskGetTickCount() * portTICK_PERIOD_MS;
    overall_result_ = TestResult::PASSED;  // Assume success until failure
    
    bool all_passed = true;
    
    for (auto& test : tests_) {
        bool test_passed = executeTest(test);
        
        if (!test_passed) {
            all_passed = false;
            updateOverallResult(test->getStatus().result);
            
            if (stop_on_first_failure_) {
                ESP_LOGE(TAG, "Stopping test execution due to failure (stop_on_first_failure enabled)");
                break;
            }
        }
    }
    
    execution_end_time_ = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    printSeparator('=', 80);
    ESP_LOGI(TAG, "                    TEST EXECUTION COMPLETED                   ");
    printSeparator('=', 80);
    
    logExecutionSummary();
    printTestSummary();
    
    return all_passed;
}

bool TestManager::runTest(const std::string& test_name)
{
    ESP_LOGI(TAG, "Running single test: %s", test_name.c_str());
    
    auto it = std::find_if(tests_.begin(), tests_.end(),
                          [&test_name](const std::shared_ptr<BaseTest>& test) {
                              return test->getName() == test_name;
                          });
    
    if (it == tests_.end()) {
        ESP_LOGE(TAG, "Test '%s' not found", test_name.c_str());
        return false;
    }
    
    execution_start_time_ = xTaskGetTickCount() * portTICK_PERIOD_MS;
    bool result = executeTest(*it);
    execution_end_time_ = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    return result;
}

bool TestManager::runTestsMatching(const std::string& pattern)
{
    ESP_LOGI(TAG, "Running tests matching pattern: %s", pattern.c_str());
    
    std::vector<std::shared_ptr<BaseTest>> matching_tests;
    
    for (auto& test : tests_) {
        if (matchesPattern(test->getName(), pattern)) {
            matching_tests.push_back(test);
        }
    }
    
    if (matching_tests.empty()) {
        ESP_LOGW(TAG, "No tests match pattern '%s'", pattern.c_str());
        return true;
    }
    
    ESP_LOGI(TAG, "Found %zu tests matching pattern", matching_tests.size());
    
    execution_start_time_ = xTaskGetTickCount() * portTICK_PERIOD_MS;
    overall_result_ = TestResult::PASSED;
    
    bool all_passed = true;
    
    for (auto& test : matching_tests) {
        bool test_passed = executeTest(test);
        
        if (!test_passed) {
            all_passed = false;
            updateOverallResult(test->getStatus().result);
            
            if (stop_on_first_failure_) {
                ESP_LOGE(TAG, "Stopping test execution due to failure");
                break;
            }
        }
    }
    
    execution_end_time_ = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    return all_passed;
}

void TestManager::printTestResults()
{
    printSeparator('-', 80);
    ESP_LOGI(TAG, "                        TEST RESULTS                          ");
    printSeparator('-', 80);
    
    for (const auto& test : tests_) {
        const TestStatus& status = test->getStatus();
        const char* icon = getResultIcon(status.result);
        const char* color = getResultColorCode(status.result);
        
        ESP_LOGI(TAG, "%s%s %-20s %s (%lu ms) - %s\033[0m",
                 color, icon, test->getName().c_str(),
                 BaseTest::resultToString(status.result).c_str(),
                 status.duration_ms,
                 status.message.c_str());
    }
    
    printSeparator('-', 80);
}

void TestManager::printTestSummary()
{
    TestStatistics stats = getStatistics();
    
    printSeparator('=', 80);
    ESP_LOGI(TAG, "                       TEST SUMMARY                          ");
    printSeparator('=', 80);
    
    ESP_LOGI(TAG, "Total Tests:     %lu", stats.total_tests);
    ESP_LOGI(TAG, "\033[32mPassed:          %lu\033[0m", stats.passed_tests);
    
    if (stats.failed_tests > 0) {
        ESP_LOGE(TAG, "\033[31mFailed:          %lu\033[0m", stats.failed_tests);
    } else {
        ESP_LOGI(TAG, "Failed:          %lu", stats.failed_tests);
    }
    
    if (stats.skipped_tests > 0) {
        ESP_LOGW(TAG, "\033[33mSkipped:         %lu\033[0m", stats.skipped_tests);
    } else {
        ESP_LOGI(TAG, "Skipped:         %lu", stats.skipped_tests);
    }
    
    if (stats.timeout_tests > 0) {
        ESP_LOGE(TAG, "\033[35mTimeout:         %lu\033[0m", stats.timeout_tests);
    } else {
        ESP_LOGI(TAG, "Timeout:         %lu", stats.timeout_tests);
    }
    
    ESP_LOGI(TAG, "Success Rate:    %.1f%%", stats.success_rate);
    ESP_LOGI(TAG, "Total Duration:  %lu ms (%.1f seconds)", stats.total_duration_ms, stats.total_duration_ms / 1000.0f);
    
    TestResult overall = getOverallResult();
    const char* overall_color = getResultColorCode(overall);
    ESP_LOGI(TAG, "%sOverall Result:  %s\033[0m", overall_color, BaseTest::resultToString(overall).c_str());
    
    printSeparator('=', 80);
}

TestResult TestManager::getOverallResult() const
{
    if (tests_.empty()) {
        return TestResult::NOT_RUN;
    }
    
    bool has_failures = false;
    bool has_timeouts = false;
    bool all_passed = true;
    
    for (const auto& test : tests_) {
        TestResult result = test->getStatus().result;
        
        switch (result) {
            case TestResult::FAILED:
                has_failures = true;
                all_passed = false;
                break;
            case TestResult::TIMEOUT:
                has_timeouts = true;
                all_passed = false;
                break;
            case TestResult::NOT_RUN:
            case TestResult::RUNNING:
            case TestResult::SKIPPED:
                all_passed = false;
                break;
            case TestResult::PASSED:
                // Continue
                break;
        }
    }
    
    if (has_failures) {
        return TestResult::FAILED;
    } else if (has_timeouts) {
        return TestResult::TIMEOUT;
    } else if (all_passed) {
        return TestResult::PASSED;
    } else {
        return TestResult::SKIPPED;  // Mixed results with no failures
    }
}

std::vector<std::string> TestManager::getTestNames() const
{
    std::vector<std::string> names;
    for (const auto& test : tests_) {
        names.push_back(test->getName());
    }
    return names;
}

BaseTest* TestManager::getTest(const std::string& name)
{
    auto it = std::find_if(tests_.begin(), tests_.end(),
                          [&name](const std::shared_ptr<BaseTest>& test) {
                              return test->getName() == name;
                          });
    
    return (it != tests_.end()) ? it->get() : nullptr;
}

TestManager::TestStatistics TestManager::getStatistics() const
{
    TestStatistics stats;
    stats.total_tests = tests_.size();
    
    for (const auto& test : tests_) {
        const TestStatus& status = test->getStatus();
        stats.total_duration_ms += status.duration_ms;
        
        switch (status.result) {
            case TestResult::PASSED:
                stats.passed_tests++;
                break;
            case TestResult::FAILED:
                stats.failed_tests++;
                break;
            case TestResult::SKIPPED:
                stats.skipped_tests++;
                break;
            case TestResult::TIMEOUT:
                stats.timeout_tests++;
                break;
            default:
                // NOT_RUN, RUNNING - don't count in specific categories
                break;
        }
    }
    
    if (stats.total_tests > 0) {
        stats.success_rate = (float)stats.passed_tests / stats.total_tests * 100.0f;
    }
    
    return stats;
}

bool TestManager::executeTest(std::shared_ptr<BaseTest> test)
{
    if (!test) {
        return false;
    }
    
    logTestStart(test->getName());
    
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Run the test
    TestResult result = test->run();
    
    uint32_t end_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t duration = end_time - start_time;
    
    logTestEnd(test->getName(), result, duration);
    
    return (result == TestResult::PASSED);
}

bool TestManager::checkTestTimeout(std::shared_ptr<BaseTest> test, uint32_t start_time)
{
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    return (current_time - start_time) > test_timeout_ms_;
}

void TestManager::updateOverallResult(TestResult test_result)
{
    if (test_result == TestResult::FAILED) {
        overall_result_ = TestResult::FAILED;
    } else if (test_result == TestResult::TIMEOUT && overall_result_ != TestResult::FAILED) {
        overall_result_ = TestResult::TIMEOUT;
    }
}

void TestManager::logTestStart(const std::string& test_name)
{
    printSeparator('-', 60);
    ESP_LOGI(TAG, "Starting test: %s", test_name.c_str());
    printSeparator('-', 60);
}

void TestManager::logTestEnd(const std::string& test_name, TestResult result, uint32_t duration_ms)
{
    const char* icon = getResultIcon(result);
    const char* color = getResultColorCode(result);
    
    ESP_LOGI(TAG, "%s%s Test '%s' %s in %lu ms\033[0m",
             color, icon, test_name.c_str(),
             BaseTest::resultToString(result).c_str(),
             duration_ms);
}

void TestManager::logExecutionSummary()
{
    uint32_t total_duration = execution_end_time_ - execution_start_time_;
    TestStatistics stats = getStatistics();
    
    ESP_LOGI(TAG, "Execution completed in %lu ms (%.1f seconds)", 
             total_duration, total_duration / 1000.0f);
    ESP_LOGI(TAG, "Tests: %lu total, %lu passed, %lu failed", 
             stats.total_tests, stats.passed_tests, stats.failed_tests);
}

bool TestManager::matchesPattern(const std::string& text, const std::string& pattern)
{
    // Simple pattern matching - could be enhanced with regex
    if (pattern == "*") {
        return true;
    }
    
    // Check if pattern is contained in text (case-insensitive)
    std::string lower_text = text;
    std::string lower_pattern = pattern;
    
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    std::transform(lower_pattern.begin(), lower_pattern.end(), lower_pattern.begin(), ::tolower);
    
    return lower_text.find(lower_pattern) != std::string::npos;
}

const char* TestManager::getResultIcon(TestResult result)
{
    switch (result) {
        case TestResult::PASSED:    return "✓";
        case TestResult::FAILED:    return "✗";
        case TestResult::RUNNING:   return "⏳";
        case TestResult::TIMEOUT:   return "⏰";
        case TestResult::SKIPPED:   return "⊝";
        case TestResult::NOT_RUN:   return "○";
        default:                    return "?";
    }
}

const char* TestManager::getResultColorCode(TestResult result)
{
    return BaseTest::getResultColor(result);
}

void TestManager::printSeparator(char character, int length)
{
    std::string separator(length, character);
    ESP_LOGI(TAG, "%s", separator.c_str());
}