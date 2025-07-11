#ifndef TEST_MANAGER_HPP
#define TEST_MANAGER_HPP

#include "base_test.hpp"
#include <vector>
#include <memory>
#include <functional>

class TestManager {
public:
    TestManager();
    ~TestManager() = default;

    // Test management
    void addTest(std::unique_ptr<BaseTest> test);
    void addTest(std::shared_ptr<BaseTest> test);
    
    // Test execution
    bool runAllTests();
    bool runTest(const std::string& test_name);
    bool runTestsMatching(const std::string& pattern);
    
    // Test control
    void setStopOnFirstFailure(bool stop) { stop_on_first_failure_ = stop; }
    void setParallelExecution(bool parallel) { parallel_execution_ = parallel; }
    void setTestTimeout(uint32_t timeout_ms) { test_timeout_ms_ = timeout_ms; }
    
    // Results and reporting
    void printTestResults();
    void printTestSummary();
    TestResult getOverallResult() const;
    
    // Test information
    size_t getTestCount() const { return tests_.size(); }
    std::vector<std::string> getTestNames() const;
    BaseTest* getTest(const std::string& name);
    
    // Statistics
    struct TestStatistics {
        uint32_t total_tests = 0;
        uint32_t passed_tests = 0;
        uint32_t failed_tests = 0;
        uint32_t skipped_tests = 0;
        uint32_t timeout_tests = 0;
        uint32_t total_duration_ms = 0;
        float success_rate = 0.0f;
    };
    
    TestStatistics getStatistics() const;

private:
    std::vector<std::shared_ptr<BaseTest>> tests_;
    bool stop_on_first_failure_;
    bool parallel_execution_;
    uint32_t test_timeout_ms_;
    
    // Results tracking
    TestResult overall_result_;
    uint32_t execution_start_time_;
    uint32_t execution_end_time_;
    
    // Helper methods
    bool executeTest(std::shared_ptr<BaseTest> test);
    bool checkTestTimeout(std::shared_ptr<BaseTest> test, uint32_t start_time);
    void updateOverallResult(TestResult test_result);
    void logTestStart(const std::string& test_name);
    void logTestEnd(const std::string& test_name, TestResult result, uint32_t duration_ms);
    void logExecutionSummary();
    
    // Pattern matching
    bool matchesPattern(const std::string& text, const std::string& pattern);
    
    // Formatting helpers
    const char* getResultIcon(TestResult result);
    const char* getResultColorCode(TestResult result);
    void printSeparator(char character = '=', int length = 80);
    
    static const char* TAG;
};

#endif // TEST_MANAGER_HPP