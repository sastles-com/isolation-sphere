#ifndef PSRAM_TEST_HPP
#define PSRAM_TEST_HPP

#include "base_test.hpp"
#include "esp_heap_caps.h"
#include "hardware_info.hpp"

class PSRAMTest : public BaseTest {
public:
    PSRAMTest();
    virtual ~PSRAMTest() = default;

    // Implement base test methods
    esp_err_t setup() override;
    esp_err_t execute() override;
    esp_err_t teardown() override;

    // PSRAM-specific methods
    esp_err_t checkPSRAMAvailability();
    esp_err_t verifyPSRAMSize();
    esp_err_t testPSRAMAllocation();
    esp_err_t testPSRAMReadWrite();
    esp_err_t measurePSRAMPerformance();

    // Configuration
    void setMinExpectedSize(size_t min_size) { min_expected_size_ = min_size; }
    void setAllocationTestSize(size_t test_size) { allocation_test_size_ = test_size; }

private:
    // Configuration
    size_t min_expected_size_;     // Minimum expected PSRAM size (bytes)
    size_t allocation_test_size_;  // Size for allocation test (bytes)
    
    // Test state
    HardwareInfo* hw_info_;
    void* test_buffer_;
    
    // Test results
    size_t psram_total_;
    size_t psram_free_;
    size_t internal_ram_total_;
    size_t internal_ram_free_;
    
    // Helper methods
    esp_err_t initializeHardwareInfo();
    esp_err_t cleanupTestBuffer();
    esp_err_t validateMemoryPattern(void* buffer, size_t size, uint8_t pattern);
    void logMemoryInfo();
};

#endif // PSRAM_TEST_HPP