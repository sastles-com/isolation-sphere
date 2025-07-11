# BNO055 IMU Driver Component

ESP-IDF component for the Bosch BNO055 9-axis absolute orientation sensor.

## Overview

The BNO055 is a System in Package (SiP), integrating a triaxial 14-bit accelerometer, a triaxial 16-bit gyroscope with a range of ±2000 degrees per second, a triaxial geomagnetic sensor and a 32-bit cortex M0+ microcontroller running Bosch Sensortec sensor fusion software, in a single package.

## Features

- I2C communication interface
- Quaternion data output
- Multiple operation modes
- Hardware reset support
- Configurable power modes

## Hardware Connections

For M5atomS3R with GROVE connector:

```
BNO055    M5atomS3R GROVE
------    ---------------
VCC   <-> 3.3V
GND   <-> GND
SDA   <-> GPIO 2 (SDA)
SCL   <-> GPIO 1 (SCL)
```

## Usage

### 1. Include the header

```c
#include "bno055.h"
```

### 2. Initialize the sensor

```c
bno055_config_t config = {
    .i2c_port = I2C_NUM_0,
    .sda_pin = GPIO_NUM_2,
    .scl_pin = GPIO_NUM_1,
    .i2c_freq = 100000,  // 100kHz
    .i2c_addr = BNO055_I2C_ADDR
};

esp_err_t ret = bno055_init(&config);
if (ret != ESP_OK) {
    ESP_LOGE("MAIN", "BNO055 initialization failed");
}
```

### 3. Read quaternion data

```c
bno055_quaternion_t quat;
esp_err_t ret = bno055_get_quaternion(&quat);
if (ret == ESP_OK) {
    ESP_LOGI("MAIN", "Quaternion: w=%.3f, x=%.3f, y=%.3f, z=%.3f", 
             quat.w, quat.x, quat.y, quat.z);
}
```

### 4. Cleanup

```c
bno055_deinit(I2C_NUM_0);
```

## API Reference

### Configuration Structure

```c
typedef struct {
    i2c_port_t i2c_port;    // I2C port number
    gpio_num_t sda_pin;     // SDA GPIO pin
    gpio_num_t scl_pin;     // SCL GPIO pin
    uint32_t i2c_freq;      // I2C frequency (Hz)
    uint8_t i2c_addr;       // BNO055 I2C address
} bno055_config_t;
```

### Quaternion Structure

```c
typedef struct {
    float w;  // Real component
    float x;  // i component
    float y;  // j component
    float z;  // k component
} bno055_quaternion_t;
```

### Functions

#### Initialization
- `esp_err_t bno055_init(const bno055_config_t *config)` - Initialize BNO055
- `esp_err_t bno055_deinit(i2c_port_t i2c_port)` - Deinitialize and cleanup
- `bool bno055_is_initialized(void)` - Check initialization status

#### Control
- `esp_err_t bno055_reset(void)` - Reset the sensor
- `esp_err_t bno055_set_mode(bno055_opmode_t mode)` - Set operation mode
- `esp_err_t bno055_set_power_mode(bno055_powermode_t power_mode)` - Set power mode

#### Data Reading
- `esp_err_t bno055_get_chip_id(uint8_t *chip_id)` - Read chip ID (should be 0xA0)
- `esp_err_t bno055_get_quaternion(bno055_quaternion_t *quat)` - Read quaternion data

### Operation Modes

The BNO055 supports multiple operation modes:

- `BNO055_OPERATION_MODE_CONFIG` - Configuration mode
- `BNO055_OPERATION_MODE_NDOF` - 9 degrees of freedom mode (recommended for quaternions)
- `BNO055_OPERATION_MODE_IMUPLUS` - IMU mode
- And more...

## Error Handling

All functions return `esp_err_t` values:
- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid arguments
- `ESP_ERR_INVALID_STATE` - Not initialized
- `ESP_ERR_NOT_FOUND` - Device not found or wrong chip ID
- Other ESP-IDF I2C error codes

## Example

See the main application for a complete example of BNO055 usage with quaternion data logging.

## Notes

- The sensor automatically performs sensor fusion and outputs quaternion data
- Default operation mode is NDOF (9 degrees of freedom) for best quaternion accuracy
- I2C frequency should not exceed 400kHz
- Allow sufficient startup time after initialization before reading data
- The quaternion values are normalized (unit quaternion representing rotation)

## Dependencies

- ESP-IDF driver component (I2C)
- ESP-IDF freertos component
- ESP-IDF esp_common component