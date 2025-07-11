# M5atomS3R ESP-IDF Project

This file provides guidance for Claude Code when working with this ESP-IDF project designed for M5atomS3R development.

## Project Overview

This is a hybrid development environment for M5atomS3R (ESP32-S3) that combines:
- **Docker-based compilation**: Clean, consistent build environment
- **Local flash/monitor**: Direct hardware access for deployment and debugging
- **Integrated workflow**: Seamless development experience

## Hardware Target

**M5atomS3R Specifications:**
- MCU: ESP32-S3FH4R2 (ESP32-S3-PICO-1)
- Flash: 8MB (4MB configured in firmware)
- PSRAM: 8MB
- Features: RGB LED, Button, IMU(BNO055), WiFi/Bluetooth, GROVE I2C connector

## Project Structure

```
isolation-sphere/
├── main/
│   ├── CMakeLists.txt
│   └── main.c                 # Main application code
├── build-logs/                # Build logs (auto-created)
├── build/                     # Build artifacts (auto-created)
├── CMakeLists.txt             # Project configuration
├── sdkconfig.defaults         # Default ESP-IDF configuration
├── docker-compose.yml         # Docker build environment
├── docker-build.sh           # Docker build wrapper
├── flash-monitor.sh          # Local flash/monitor tool
├── dev.sh                    # Integrated development workflow
└── CLAUDE.md                 # This file
```

## Development Workflow

### Quick Start
```bash
# Complete workflow: build, flash, and monitor
./dev.sh

# Or step by step:
./dev.sh build-flash-monitor
```

### Individual Commands

#### Building (Docker)
```bash
# Build project
./docker-build.sh build

# Clean build
./docker-build.sh clean

# Full clean
./docker-build.sh fullclean

# Configuration menu
./docker-build.sh menuconfig

# Interactive shell
./docker-build.sh shell
```

#### Flashing/Monitoring (Local)
```bash
# Auto-detect port and flash
./flash-monitor.sh flash

# Flash and monitor
./flash-monitor.sh flash-monitor

# Monitor only
./flash-monitor.sh monitor

# Specify port
./flash-monitor.sh flash /dev/ttyUSB0

# Detect available ports
./flash-monitor.sh detect
```

#### Integrated Workflow
```bash
# Build and flash
./dev.sh build-flash

# Build and monitor
./dev.sh build-monitor

# Complete workflow
./dev.sh build-flash-monitor

# Flash without rebuilding
./dev.sh quick-flash

# Project status
./dev.sh status

# Clean project
./dev.sh clean
```

## Configuration

### ESP-IDF Configuration
Key settings in `sdkconfig.defaults`:
- Target: ESP32-S3
- Flash: 4MB, 80MHz, QIO mode
- PSRAM: 8MB, OCT mode, 40MHz, enabled with malloc support
- WiFi/Bluetooth: enabled
- USB Serial/JTAG: enabled for debugging

### Docker Configuration
- Base image: `espressif/idf:release-v5.0`
- Volume mounts: project source, build logs, tool cache
- Network: host mode for device access

### GPIO Mapping (M5atomS3R)
```c
#define LED_GPIO    GPIO_NUM_35      // RGB LED (WS2812)
#define BUTTON_GPIO GPIO_NUM_41      // Button
#define SDA_GPIO    GPIO_NUM_2       // I2C SDA (GROVE connector)
#define SCL_GPIO    GPIO_NUM_1       // I2C SCL (GROVE connector)
```

## Troubleshooting

### Build Issues
1. Check Docker is running: `docker info`
2. Clean build: `./docker-build.sh fullclean`
3. Check logs: `ls -la build-logs/`

### Flash Issues
1. Check device connection: `./flash-monitor.sh detect`
2. Check permissions: `ls -la /dev/ttyUSB*`
3. Try different baud rate in script

### Monitor Issues
1. Install terminal program: `brew install screen` (macOS)
2. Check port permissions
3. Verify device is not in use by other applications

## Development Tips

### Code Development
- Edit `main/main.c` for application logic
- Use `./dev.sh build-flash-monitor` for complete workflow
- Press Ctrl+C to exit monitor

### Debugging
- Use `ESP_LOGI(TAG, "message")` for logging
- Monitor serial output at 115200 baud
- Check build logs in `build-logs/` directory

### Adding Libraries
1. Add to `main/CMakeLists.txt`:
   ```cmake
   idf_component_register(SRCS "main.c"
                         INCLUDE_DIRS "."
                         REQUIRES driver esp_wifi)
   ```

2. Include in `main/main.c`:
   ```c
   #include "driver/gpio.h"
   #include "esp_wifi.h"
   ```

## Best Practices

1. **Use the integrated workflow**: `./dev.sh` provides the most efficient development experience
2. **Check status regularly**: `./dev.sh status` shows project health
3. **Clean builds when switching targets**: `./docker-build.sh fullclean`
4. **Monitor logs**: Check `build-logs/` for build issues
5. **Version control**: Add `build/` and `build-logs/` to `.gitignore`

## Common Commands

```bash
# Daily development
./dev.sh                          # Build, flash, monitor
./dev.sh quick-flash              # Flash without rebuild
./dev.sh status                   # Check project status

# Configuration
./docker-build.sh menuconfig      # ESP-IDF configuration
./docker-build.sh shell           # Docker shell access

# Debugging
./flash-monitor.sh detect         # Find serial ports
./docker-build.sh size            # Check binary size
tail -f build-logs/build_*.log    # Watch build logs
```

## PSRAM Configuration and Testing

### PSRAM Setup Success
The M5atomS3R's 8MB PSRAM has been successfully configured and verified:

**Verification Steps:**
1. **Configuration Reset**: Removed old `sdkconfig` and rebuilt with fresh settings
2. **Correct Settings Applied**: 
   - `CONFIG_SPIRAM=y` - PSRAM enabled
   - `CONFIG_SPIRAM_MODE_OCT=y` - OCT mode for high performance
   - `CONFIG_SPIRAM_TYPE_ESPPSRAM64=y` - 64Mbit (8MB) type
   - `CONFIG_SPIRAM_SPEED_40M=y` - 40MHz for stability
   - `CONFIG_SPIRAM_USE_MALLOC=y` - Enable malloc() usage

**Test Results (Successful):**
```
I (1008) esp_psram: SPI SRAM memory test OK
I (1020) esp_psram: Adding pool of 8192K of PSRAM memory to heap allocator
I (1025) M5ATOMS3R: PSRAM total: 8388608 bytes
I (1025) M5ATOMS3R: PSRAM free: 8386308 bytes
I (1025) M5ATOMS3R: Free Heap: 8734192 bytes (PSRAM + Internal RAM)
```

### Monitoring Tools
For stable log monitoring during development:
```bash
# Continuous monitor (auto-reconnect on reset)
./monitor-loop.sh

# Screen-based monitor (reset-resistant)
./monitor-reset.sh
```

## Test Development Policy

**⚠️ IMPORTANT: All tests must be implemented as classes in the test framework.**

All hardware and functionality tests should be implemented using the class-based test framework located in `components/test_framework/`. This ensures:
- **Consistency**: Standardized test structure and reporting
- **Maintainability**: Easy to modify and extend existing tests
- **Reusability**: Test components can be reused across different scenarios
- **Professional Output**: Comprehensive test results with detailed statistics

### Adding New Tests
1. Create a new test class inheriting from `BaseTest`
2. Implement `setup()`, `execute()`, and `teardown()` methods
3. Add test-specific configuration methods
4. Register the test with `TestManager` in `test_main.cpp`

## Test Plan

### Test01: PSRAM Verification ✅ COMPLETED
- **Implementation**: `PSRAMTest` class
- **Status**: PASSED
- **PSRAM Total**: 8,388,608 bytes (8MB)
- **PSRAM Free**: 8,386,308 bytes
- **Heap Total**: 8,734,192 bytes (PSRAM + Internal RAM)

### Test02: IMU (BNO055) Operation Test ✅ COMPLETED
- **Implementation**: `BNO055Test` class
- **Objective**: Verify BNO055 IMU functionality
- **Connection**: I2C via GROVE connector (SDA:2, SCL:1)
- **Target Data**: Quaternion values with validation
- **Status**: IMPLEMENTED

### Test03: WiFi Connection Test ✅ COMPLETED
- **Implementation**: `WiFiTest` class
- **Objective**: Verify WiFi connectivity and stability
- **Target Network**: `ros2_atom_ap` / `isolation-sphere`
- **Tests**: Connection, stability, scan, reconnection, performance
- **Status**: IMPLEMENTED

### Test04: ROS2 Connection Test 🔄 PENDING
- **Implementation**: `ROS2Test` class (to be created)
- **Objective**: Verify ROS2 communication over WiFi
- **Requirements**: 
  - WiFi connection to `ros2_atom_ap`
  - ROS2 node creation and topic communication
  - IMU data publishing to `m5atom/imu` topic
  - Video frame subscription from `video_frames` topic

## ROS2 Communication System

The project includes a Raspberry Pi-based ROS2 communication system that the ESP32 connects to:

### Raspberry Pi ROS2 Components

#### 1. Main Web Server (`raspi/main.py`)
- **Purpose**: FastAPI web server handling video uploads and ROS2 bridge
- **Functionality**:
  - Video processing (320x160 MP4 at 10fps)
  - Redis middleware for data exchange
  - ROS2 publisher for video frames
- **ROS2 Integration**:
  - Node: `web_frame_bridge`
  - Publisher: `video_frames` (sensor_msgs/CompressedImage)

#### 2. ROS2 Bridge (`raspi/ros2_bridge.py`)
- **Purpose**: Core bidirectional communication between Redis and ROS2
- **Functionality**:
  - Frame publishing at 10Hz from Redis to ROS2
  - IMU data subscription and storage to Redis
- **ROS2 Topics**:
  - **Publisher**: `video_frames` (sensor_msgs/CompressedImage)
  - **Subscriber**: `m5atom/imu` (sensor_msgs/Imu)

#### 3. Frame Subscriber (`raspi/subscriber.py`)
- **Purpose**: Test subscriber for video frame reception
- **Functionality**:
  - Subscribes to compressed image frames
  - Debug output and optional frame saving
- **ROS2 Integration**:
  - Node: `frame_subscriber`
  - Subscriber: `video_frames` (sensor_msgs/CompressedImage)

### ESP32 ROS2 Integration Requirements

For Test04 implementation, the ESP32 needs to:

#### 1. IMU Data Publishing
- **Topic**: `m5atom/imu`
- **Message Type**: `sensor_msgs/Imu`
- **Data**: Full IMU message with orientation quaternion
- **Frequency**: 10Hz (to match bridge processing rate)

#### 2. Video Frame Subscription
- **Topic**: `video_frames`
- **Message Type**: `sensor_msgs/CompressedImage`
- **Format**: JPEG compressed images (320x160 pixels)
- **Frame Rate**: 10fps

#### 3. Data Formats
- **IMU Quaternion**: Standard geometry_msgs/Quaternion (x, y, z, w)
- **Images**: JPEG compressed in CompressedImage.data field
- **Communication**: Direct ROS2 topic communication (no Redis on ESP32)

#### 4. Communication Flow
```
ESP32 → ROS2 → Raspberry Pi
[BNO055] → [m5atom/imu] → [ros2_bridge.py] → [Redis]

Raspberry Pi → ROS2 → ESP32  
[video processing] → [video_frames] → [ESP32 subscriber]
```

This setup provides a professional development environment with clean separation between build and deployment phases, comprehensive logging, integrated workflow automation, verified 8MB PSRAM functionality, and a complete class-based test framework for systematic hardware validation.