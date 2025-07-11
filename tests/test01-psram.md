# Test01: PSRAM Verification

## Overview

Verification of M5atomS3R's 8MB PSRAM initialization, configuration, and memory allocation functionality.

## Test Objectives

- [x] Verify PSRAM hardware detection
- [x] Confirm 8MB PSRAM capacity
- [x] Test PSRAM initialization process
- [x] Validate memory allocation via malloc()
- [x] Check heap integration (PSRAM + Internal RAM)

## Hardware Requirements

- M5atomS3R device with 8MB PSRAM
- USB-C cable for power and programming
- Host computer with ESP-IDF development environment

## Test Configuration

### ESP-IDF Settings (`sdkconfig.defaults`)

```ini
# PSRAM Configuration (M5atomS3R - 8MB PSRAM)
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_TYPE_ESPPSRAM64=y
CONFIG_SPIRAM_SPEED_40M=y
CONFIG_SPIRAM_BOOT_INIT=y
CONFIG_SPIRAM_IGNORE_NOTFOUND=n
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384
CONFIG_SPIRAM_CACHE_WORKAROUND=y
CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y
```

### Key Configuration Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| `CONFIG_SPIRAM` | y | Enable PSRAM support |
| `CONFIG_SPIRAM_MODE_OCT` | y | OCT mode for high performance |
| `CONFIG_SPIRAM_TYPE_ESPPSRAM64` | y | 64Mbit (8MB) PSRAM type |
| `CONFIG_SPIRAM_SPEED_40M` | y | 40MHz clock for stability |
| `CONFIG_SPIRAM_USE_MALLOC` | y | Enable malloc() allocation |

## Test Procedure

### 1. Preparation

```bash
# Clean build environment
./docker-build.sh fullclean
rm -f sdkconfig

# Build and flash
./dev.sh build-flash
```

### 2. Monitor Startup Logs

```bash
# Start monitoring with auto-reconnect
./monitor-loop.sh
```

### 3. Reset Device

Press the reset button on M5atomS3R to capture complete boot sequence.

## Expected Results

### Successful PSRAM Initialization

```
I (1008) esp_psram: SPI SRAM memory test OK
I (1020) esp_psram: Adding pool of 8192K of PSRAM memory to heap allocator
I (1024) esp_psram: Reserving pool of 32K of internal memory for DMA/internal allocations
```

### Memory Information Output

```
I (1025) M5ATOMS3R: === PSRAM Information ===
I (1025) M5ATOMS3R: PSRAM total: 8388608 bytes
I (1025) M5ATOMS3R: PSRAM free: 8386308 bytes
I (1026) M5ATOMS3R: Internal RAM total: 438219 bytes
I (1026) M5ATOMS3R: Internal RAM free: 379915 bytes
I (1025) M5ATOMS3R: Free Heap: 8734192 bytes
```

### Hardware Information Report

```
=== M5atomS3R Hardware Information ===
Chip Model: ESP32-S3
Chip Revision: v0.2
MAC Address: f0:9e:9e:32:8e:34
Flash Size: 4 MB
PSRAM Enabled: Yes
PSRAM Initialized: Yes
PSRAM Size: 8 MB
PSRAM Total: 8388608 bytes
PSRAM Free: 8386308 bytes
Free Heap: 8722840 bytes
Total Heap: 8794060 bytes
```

## Success Criteria

| Criteria | Expected Value | Status |
|----------|----------------|--------|
| PSRAM Test Result | "SPI SRAM memory test OK" | ✅ PASS |
| PSRAM Total Size | 8,388,608 bytes (8MB) | ✅ PASS |
| PSRAM Free Size | >8,300,000 bytes | ✅ PASS |
| Heap Integration | >8,700,000 bytes total | ✅ PASS |
| Memory Allocation | malloc() uses PSRAM | ✅ PASS |

## Troubleshooting

### Common Issues

1. **PSRAM not detected**
   - Check hardware revision (must support PSRAM)
   - Verify configuration settings
   - Ensure clean build after config changes

2. **PSRAM size mismatch**
   - Verify `CONFIG_SPIRAM_TYPE_ESPPSRAM64` setting
   - Check for hardware variants

3. **Memory allocation issues**
   - Confirm `CONFIG_SPIRAM_USE_MALLOC=y`
   - Check DMA reservation settings

### Resolution Steps

```bash
# Reset configuration
./docker-build.sh fullclean
rm -f sdkconfig

# Rebuild with clean config
./dev.sh build-flash

# Monitor complete boot sequence
./monitor-reset.sh
```

## Test Results

**Status**: ✅ PASSED  
**Date**: 2025-07-11  
**PSRAM Size**: 8,388,608 bytes  
**Free Heap**: 8,734,192 bytes  
**Notes**: PSRAM successfully initialized with OCT mode at 40MHz

## Implementation Details

### Code Location

- Main application: `main/main.cpp`
- Hardware info component: `components/hardware_test/`
- Configuration: `sdkconfig.defaults`

### Key Functions

```cpp
// PSRAM information display
ESP_LOGI(TAG, "PSRAM total: %d bytes", heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
ESP_LOGI(TAG, "PSRAM free: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
ESP_LOGI(TAG, "Free Heap: %" PRIu32 " bytes", esp_get_free_heap_size());
```

### Monitoring Tools

- `./monitor-loop.sh` - Continuous monitoring with auto-reconnect
- `./monitor-reset.sh` - Screen-based monitoring for reset capture

## Related Documentation

- [ESP-IDF PSRAM Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/external-ram.html)
- M5atomS3R hardware specifications
- Project main documentation: `CLAUDE.md`