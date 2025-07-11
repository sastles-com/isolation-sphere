# M5atomS3R Hardware Tests

This directory contains test documentation and procedures for M5atomS3R hardware verification.

## Test Overview

| Test ID | Name | Status | Description |
|---------|------|--------|-------------|
| Test01 | PSRAM Verification | ✅ PASSED | 8MB PSRAM initialization and memory allocation test |
| Test02 | BNO055 IMU Test | 🔧 READY | I2C IMU sensor quaternion data acquisition test |
| Test03 | WiFi Connection Test | 🔧 READY | WiFi connectivity to ros2_atom_ap network test |

## Test Structure

Each test has its own documentation:
- `test01-psram.md` - PSRAM verification procedures and results
- `test02-bno055.md` - BNO055 IMU testing procedures and expected outputs
- `test03-wifi.md` - WiFi connection testing procedures and network requirements

## Running Tests

1. Build and flash the project: `./dev.sh build-flash`
2. Monitor output: `./monitor-loop.sh` or `./monitor-reset.sh`
3. Follow individual test procedures in respective markdown files

## Test Environment

- **Hardware**: M5atomS3R (ESP32-S3FH4R2)
- **Framework**: ESP-IDF v5.0.8
- **Build**: Docker-based compilation
- **Monitoring**: Serial @ 115200 baud

## Test Results Location

- Build logs: `build-logs/`
- Serial logs: Captured via monitoring scripts
- Test status: Updated in `CLAUDE.md` main documentation