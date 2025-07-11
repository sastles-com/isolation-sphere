#include "hardware_info.hpp"
#include <esp_system.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_mac.h>
#include <sstream>
#include <iomanip>

// ESP-IDF v5.0でのPSRAMサポート
#include <esp_psram.h>

// PSRAMの利用可能性をチェックする追加関数（将来の拡張用）
// static bool is_psram_available() {
//     #ifdef CONFIG_SPIRAM
//         return true;
//     #else
//         return false;
//     #endif
// }

HardwareInfo::HardwareInfo() : initialized_(false) {
    initialized_ = init();
}

HardwareInfo::~HardwareInfo() {
    // デストラクタ
}

bool HardwareInfo::init() {
    // ESP-IDFシステムが既に初期化済みと仮定
    return true;
}

bool HardwareInfo::isInitialized() const {
    return initialized_;
}

bool HardwareInfo::getChipInfo(ChipInfo& info) {
    if (!initialized_) return false;

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    // チップモデル判定
    switch (chip_info.model) {
        case CHIP_ESP32S3:
            info.model = "ESP32-S3";
            break;
        case CHIP_ESP32:
            info.model = "ESP32";
            break;
        case CHIP_ESP32S2:
            info.model = "ESP32-S2";
            break;
        default:
            info.model = "Unknown";
            break;
    }

    info.revision = chip_info.revision;

    // MAC アドレス取得
    uint8_t mac[6];
    esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (ret == ESP_OK) {
        std::stringstream ss;
        for (int i = 0; i < 6; i++) {
            if (i > 0) ss << ":";
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(mac[i]);
        }
        info.mac_address = ss.str();
    } else {
        info.mac_address = "Unknown";
    }

    // クリスタル周波数取得（ESP-IDF v5.0対応）
    info.crystal_freq_mhz = 40; // M5atomS3Rは40MHz固定

    return true;
}

bool HardwareInfo::getMemoryInfo(MemoryInfo& info) {
    if (!initialized_) return false;

    // フラッシュサイズ取得
    uint32_t flash_size;
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        info.flash_size_mb = flash_size / (1024 * 1024);
    } else {
        info.flash_size_mb = 0;
    }

    // PSRAM詳細情報取得（ESP-IDF v5.0対応）
    info.psram_size_mb = 0;
    info.psram_free_bytes = 0;
    info.psram_total_bytes = 0;
    info.psram_enabled = false;
    info.psram_initialized = false;
    
    #ifdef CONFIG_SPIRAM
        info.psram_enabled = true;
        
        // Method 1: esp_psram_get_size() (ESP-IDF v5.0)
        size_t psram_size = esp_psram_get_size();
        if (psram_size > 0) {
            info.psram_size_mb = psram_size / (1024 * 1024);
            info.psram_initialized = true;
        }
    #endif
    
    // Method 2: heap_caps_get_total_size()でPSRAMヒープ容量を取得
    size_t spiram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    
    if (spiram_total > 0) {
        info.psram_total_bytes = spiram_total;
        info.psram_free_bytes = spiram_free;
        // heap_caps_get_total_sizeの結果も確認してより正確な値を使用
        uint32_t heap_based_mb = spiram_total / (1024 * 1024);
        if (heap_based_mb > info.psram_size_mb) {
            info.psram_size_mb = heap_based_mb;
        }
        info.psram_initialized = true;
    }

    // ヒープ情報取得
    info.free_heap_bytes = esp_get_free_heap_size();
    info.total_heap_bytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);

    return true;
}

bool HardwareInfo::getPowerInfo(PowerInfo& info) {
    if (!initialized_) return false;

    // 簡易電圧測定（実際のハードウェアに依存）
    // M5atomS3Rでは外部電圧測定回路が必要
    info.voltage_v = 3.3f; // 仮の値
    info.is_battery_powered = false; // USB給電と仮定

    return true;
}

std::string HardwareInfo::getAllInfoAsString() {
    if (!initialized_) return "Hardware not initialized";

    std::stringstream ss;
    
    ChipInfo chip_info;
    MemoryInfo mem_info;
    PowerInfo power_info;

    ss << "=== M5atomS3R Hardware Information ===\n";

    if (getChipInfo(chip_info)) {
        ss << "Chip Model: " << chip_info.model << "\n";
        ss << "Chip Revision: v" << static_cast<int>(chip_info.revision / 100) 
           << "." << static_cast<int>(chip_info.revision % 100) << "\n";
        ss << "MAC Address: " << chip_info.mac_address << "\n";
        ss << "Crystal Frequency: " << chip_info.crystal_freq_mhz << " MHz\n";
    }

    if (getMemoryInfo(mem_info)) {
        ss << "Flash Size: " << mem_info.flash_size_mb << " MB\n";
        ss << "PSRAM Enabled: " << (mem_info.psram_enabled ? "Yes" : "No") << "\n";
        ss << "PSRAM Initialized: " << (mem_info.psram_initialized ? "Yes" : "No") << "\n";
        ss << "PSRAM Size: " << mem_info.psram_size_mb << " MB\n";
        if (mem_info.psram_total_bytes > 0) {
            ss << "PSRAM Total: " << mem_info.psram_total_bytes << " bytes\n";
            ss << "PSRAM Free: " << mem_info.psram_free_bytes << " bytes\n";
            ss << "PSRAM Used: " << (mem_info.psram_total_bytes - mem_info.psram_free_bytes) << " bytes\n";
        }
        ss << "Free Heap: " << mem_info.free_heap_bytes << " bytes\n";
        ss << "Total Heap: " << mem_info.total_heap_bytes << " bytes\n";
    }

    if (getPowerInfo(power_info)) {
        ss << "Voltage: " << std::fixed << std::setprecision(2) 
           << power_info.voltage_v << " V\n";
        ss << "Battery Powered: " << (power_info.is_battery_powered ? "Yes" : "No") << "\n";
    }

    ss << "=====================================\n";

    return ss.str();
}