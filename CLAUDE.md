# M5atomS3R ESP-IDF Project

This file provides guidance for Claude Code when working with this ESP-IDF project designed for M5atomS3R development.

## Project Overview

This is a hybrid development environment for M5atomS3R (ESP32-S3) that combines:
- **Docker-based compilation**: Clean, consistent build environment
- **Local flash/monitor**: Direct hardware access for deployment and debugging
- **Integrated workflow**: Seamless development experience

## Hardware Target

**M5atomS3R Specifications:**
- MCU: ESP32-S3FH4R2
- Flash: 4MB
- PSRAM: 2MB
- Features: RGB LED, Button, IMU, WiFi/Bluetooth

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
- PSRAM: 2MB, enabled
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
#define SDA_GPIO    GPIO_NUM_38      // I2C SDA
#define SCL_GPIO    GPIO_NUM_39      // I2C SCL
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

This setup provides a professional development environment with clean separation between build and deployment phases, comprehensive logging, and integrated workflow automation.