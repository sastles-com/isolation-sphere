#pragma once

#include <string>
#include <cstdint>

/**
 * @brief M5atomS3R ハードウェア情報管理クラス
 */
class HardwareInfo {
public:
    struct ChipInfo {
        std::string model;
        uint8_t revision;
        std::string mac_address;
        uint32_t crystal_freq_mhz;
    };

    struct MemoryInfo {
        uint32_t flash_size_mb;
        uint32_t psram_size_mb;
        uint32_t psram_free_bytes;
        uint32_t psram_total_bytes;
        uint32_t free_heap_bytes;
        uint32_t total_heap_bytes;
        bool psram_enabled;
        bool psram_initialized;
    };

    struct PowerInfo {
        float voltage_v;
        bool is_battery_powered;
    };

    HardwareInfo();
    ~HardwareInfo();

    // チップ情報取得
    bool getChipInfo(ChipInfo& info);
    
    // メモリ情報取得
    bool getMemoryInfo(MemoryInfo& info);
    
    // 電源情報取得
    bool getPowerInfo(PowerInfo& info);
    
    // ハードウェア初期化状態確認
    bool isInitialized() const;
    
    // 全情報を文字列で出力
    std::string getAllInfoAsString();

private:
    bool initialized_;
    bool init();
};