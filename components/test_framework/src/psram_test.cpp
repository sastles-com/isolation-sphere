#include "psram_test.hpp"
#include <cstring>
#include <cstdlib>

PSRAMTest::PSRAMTest() 
    : BaseTest("PSRAM", "PSRAM memory verification and performance test"),
      min_expected_size_(8 * 1024 * 1024),  // 8MB default
      allocation_test_size_(1024 * 1024),   // 1MB default test size
      hw_info_(nullptr),
      test_buffer_(nullptr),
      psram_total_(0),
      psram_free_(0),
      internal_ram_total_(0),
      internal_ram_free_(0)
{
}

esp_err_t PSRAMTest::setup()
{
    logInfo("Setting up PSRAM test environment");
    
    // Initialize hardware info helper
    esp_err_t ret = initializeHardwareInfo();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Add test steps
    addStep("Check PSRAM availability", [this]() { return checkPSRAMAvailability(); });
    addStep("Verify PSRAM size", [this]() { return verifyPSRAMSize(); });
    addStep("Test PSRAM allocation", [this]() { return testPSRAMAllocation(); });
    addStep("Test PSRAM read/write", [this]() { return testPSRAMReadWrite(); });
    addStep("Measure PSRAM performance", [this]() { return measurePSRAMPerformance(); });
    
    logPass("PSRAM test setup completed");
    return ESP_OK;
}

esp_err_t PSRAMTest::execute()
{
    logInfo("Executing PSRAM test steps");
    
    // Log initial memory state
    logMemoryInfo();
    
    // Run all test steps
    esp_err_t ret = runSteps();
    if (ret != ESP_OK) {
        logError("PSRAM test execution failed");
        return ret;
    }
    
    // Log final memory state
    logInfo("Final memory state:");
    logMemoryInfo();
    
    logPass("PSRAM test execution completed successfully");
    return ESP_OK;
}

esp_err_t PSRAMTest::teardown()
{
    logInfo("Cleaning up PSRAM test");
    
    // Clean up test buffer
    esp_err_t ret = cleanupTestBuffer();
    if (ret != ESP_OK) {
        logError("Failed to cleanup test buffer");
    }
    
    // Delete hardware info instance
    if (hw_info_) {
        delete hw_info_;
        hw_info_ = nullptr;
    }
    
    logPass("PSRAM test cleanup completed");
    return ESP_OK;
}

esp_err_t PSRAMTest::checkPSRAMAvailability()
{
    logInfo("Checking PSRAM availability");
    
    // Check if PSRAM is available
    psram_total_ = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    
    TEST_ASSERT(psram_total_ > 0, "PSRAM not available or not configured");
    
    logPass("PSRAM is available: %zu bytes total", psram_total_);
    return ESP_OK;
}

esp_err_t PSRAMTest::verifyPSRAMSize()
{
    logInfo("Verifying PSRAM size meets minimum requirements");
    
    TEST_ASSERT(psram_total_ >= min_expected_size_, 
                "PSRAM size below minimum requirement");
    
    // Update memory info
    psram_free_ = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    internal_ram_total_ = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    internal_ram_free_ = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    
    logPass("PSRAM size verification passed");
    logInfo("PSRAM: %zu/%zu bytes (free/total)", psram_free_, psram_total_);
    logInfo("Internal RAM: %zu/%zu bytes (free/total)", internal_ram_free_, internal_ram_total_);
    
    return ESP_OK;
}

esp_err_t PSRAMTest::testPSRAMAllocation()
{
    logInfo("Testing PSRAM allocation with %zu bytes", allocation_test_size_);
    
    // Allocate test buffer from PSRAM
    test_buffer_ = heap_caps_malloc(allocation_test_size_, MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_NULL(test_buffer_, "Failed to allocate PSRAM buffer");
    
    // Verify allocation is actually in PSRAM
    size_t psram_free_after = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    TEST_ASSERT(psram_free_after < psram_free_, "PSRAM free size should decrease after allocation");
    
    size_t allocated_size = psram_free_ - psram_free_after;
    logPass("Successfully allocated %zu bytes in PSRAM", allocated_size);
    
    return ESP_OK;
}

esp_err_t PSRAMTest::testPSRAMReadWrite()
{
    logInfo("Testing PSRAM read/write operations");
    
    TEST_ASSERT_NOT_NULL(test_buffer_, "Test buffer not allocated");
    
    // Test pattern 1: 0xAA
    logInfo("Testing pattern 0xAA");
    memset(test_buffer_, 0xAA, allocation_test_size_);
    TEST_ASSERT_OK(validateMemoryPattern(test_buffer_, allocation_test_size_, 0xAA));
    
    // Test pattern 2: 0x55
    logInfo("Testing pattern 0x55");
    memset(test_buffer_, 0x55, allocation_test_size_);
    TEST_ASSERT_OK(validateMemoryPattern(test_buffer_, allocation_test_size_, 0x55));
    
    // Test pattern 3: 0x00
    logInfo("Testing pattern 0x00");
    memset(test_buffer_, 0x00, allocation_test_size_);
    TEST_ASSERT_OK(validateMemoryPattern(test_buffer_, allocation_test_size_, 0x00));
    
    // Test pattern 4: 0xFF
    logInfo("Testing pattern 0xFF");
    memset(test_buffer_, 0xFF, allocation_test_size_);
    TEST_ASSERT_OK(validateMemoryPattern(test_buffer_, allocation_test_size_, 0xFF));
    
    logPass("PSRAM read/write test completed successfully");
    return ESP_OK;
}

esp_err_t PSRAMTest::measurePSRAMPerformance()
{
    logInfo("Measuring PSRAM performance");
    
    TEST_ASSERT_NOT_NULL(test_buffer_, "Test buffer not allocated");
    
    // Measure write performance
    uint32_t start_time = xTaskGetTickCount();
    for (int i = 0; i < 10; i++) {
        memset(test_buffer_, i & 0xFF, allocation_test_size_);
    }
    uint32_t write_time = (xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS;
    
    // Measure read performance  
    volatile uint32_t checksum = 0;
    start_time = xTaskGetTickCount();
    for (int i = 0; i < 10; i++) {
        uint8_t* ptr = (uint8_t*)test_buffer_;
        for (size_t j = 0; j < allocation_test_size_; j += 4) {
            uint32_t value = *(uint32_t*)(ptr + j);
            checksum = checksum + value;
        }
    }
    uint32_t read_time = (xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS;
    
    // Calculate throughput
    size_t total_bytes = allocation_test_size_ * 10;
    float write_mbps = (float)(total_bytes) / (write_time * 1000.0f);  // MB/s
    float read_mbps = (float)(total_bytes) / (read_time * 1000.0f);    // MB/s
    
    logPass("PSRAM Performance Results:");
    logPass("Write: %.2f MB/s (%lu ms for %zu bytes)", write_mbps, write_time, total_bytes);
    logPass("Read: %.2f MB/s (%lu ms for %zu bytes)", read_mbps, read_time, total_bytes);
    logPass("Checksum: 0x%08lX", checksum);
    
    return ESP_OK;
}

esp_err_t PSRAMTest::initializeHardwareInfo()
{
    hw_info_ = new HardwareInfo();
    if (!hw_info_) {
        logError("Failed to allocate HardwareInfo instance");
        return ESP_ERR_NO_MEM;
    }
    
    if (!hw_info_->isInitialized()) {
        logError("Failed to initialize HardwareInfo");
        delete hw_info_;
        hw_info_ = nullptr;
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t PSRAMTest::cleanupTestBuffer()
{
    if (test_buffer_) {
        heap_caps_free(test_buffer_);
        test_buffer_ = nullptr;
        logInfo("Test buffer freed");
    }
    return ESP_OK;
}

esp_err_t PSRAMTest::validateMemoryPattern(void* buffer, size_t size, uint8_t pattern)
{
    uint8_t* ptr = (uint8_t*)buffer;
    for (size_t i = 0; i < size; i++) {
        if (ptr[i] != pattern) {
            logError("Memory pattern validation failed at offset %zu: expected 0x%02X, got 0x%02X", 
                     i, pattern, ptr[i]);
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

void PSRAMTest::logMemoryInfo()
{
    if (hw_info_) {
        HardwareInfo::MemoryInfo mem_info;
        if (hw_info_->getMemoryInfo(mem_info)) {
            logInfo("Memory Status:");
            logInfo("  Total Heap: %lu bytes", mem_info.total_heap_bytes);
            logInfo("  Free Heap: %lu bytes", mem_info.free_heap_bytes);
            logInfo("  PSRAM Total: %lu bytes", mem_info.psram_total_bytes);
            logInfo("  PSRAM Free: %lu bytes", mem_info.psram_free_bytes);
            logInfo("  Internal RAM Total: %zu bytes", heap_caps_get_total_size(MALLOC_CAP_INTERNAL));
            logInfo("  Internal RAM Free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        }
    } else {
        // Fallback to direct heap calls
        logInfo("Memory Status (direct):");
        logInfo("  PSRAM Total: %zu bytes", heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
        logInfo("  PSRAM Free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        logInfo("  Internal RAM Total: %zu bytes", heap_caps_get_total_size(MALLOC_CAP_INTERNAL));
        logInfo("  Internal RAM Free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    }
}