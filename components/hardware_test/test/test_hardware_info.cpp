#include "unity.h"
#include "hardware_info.hpp"

void setUp(void) {
    // テスト前の初期化
}

void tearDown(void) {
    // テスト後のクリーンアップ
}

void test_hardware_info_creation() {
    HardwareInfo hw_info;
    TEST_ASSERT_TRUE(hw_info.isInitialized());
}

void test_chip_info_retrieval() {
    HardwareInfo hw_info;
    HardwareInfo::ChipInfo chip_info;
    
    bool result = hw_info.getChipInfo(chip_info);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(chip_info.model.empty());
    TEST_ASSERT_GREATER_THAN(0, chip_info.revision);
    TEST_ASSERT_FALSE(chip_info.mac_address.empty());
    TEST_ASSERT_GREATER_THAN(0, chip_info.crystal_freq_mhz);
}

void test_memory_info_retrieval() {
    HardwareInfo hw_info;
    HardwareInfo::MemoryInfo mem_info;
    
    bool result = hw_info.getMemoryInfo(mem_info);
    TEST_ASSERT_TRUE(result);
    
    // M5atomS3Rの期待値をテスト
    TEST_ASSERT_GREATER_OR_EQUAL(4, mem_info.flash_size_mb);  // 最低4MB
    TEST_ASSERT_GREATER_OR_EQUAL(2, mem_info.psram_size_mb);  // 最低2MB
    TEST_ASSERT_GREATER_THAN(0, mem_info.free_heap_bytes);
    TEST_ASSERT_GREATER_THAN(0, mem_info.total_heap_bytes);
    TEST_ASSERT_LESS_THAN(mem_info.total_heap_bytes, mem_info.free_heap_bytes);
}

void test_power_info_retrieval() {
    HardwareInfo hw_info;
    HardwareInfo::PowerInfo power_info;
    
    bool result = hw_info.getPowerInfo(power_info);
    TEST_ASSERT_TRUE(result);
    
    // 電圧は2.5V~5.5Vの範囲であるべき
    TEST_ASSERT_GREATER_THAN(2.5f, power_info.voltage_v);
    TEST_ASSERT_LESS_THAN(5.5f, power_info.voltage_v);
}

void test_all_info_string_output() {
    HardwareInfo hw_info;
    std::string info_str = hw_info.getAllInfoAsString();
    
    TEST_ASSERT_FALSE(info_str.empty());
    
    // 重要な情報が含まれているかチェック
    TEST_ASSERT_TRUE(info_str.find("ESP32") != std::string::npos);
    TEST_ASSERT_TRUE(info_str.find("Flash") != std::string::npos);
    TEST_ASSERT_TRUE(info_str.find("PSRAM") != std::string::npos);
    TEST_ASSERT_TRUE(info_str.find("MAC") != std::string::npos);
}

// Unity テストランナー関数
extern "C" void app_main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_hardware_info_creation);
    RUN_TEST(test_chip_info_retrieval);
    RUN_TEST(test_memory_info_retrieval);
    RUN_TEST(test_power_info_retrieval);
    RUN_TEST(test_all_info_string_output);
    
    UNITY_END();
}