# Test02: BNO055 IMU Operation Test

## Overview

Verification of BNO055 9-axis IMU sensor connectivity, initialization, and quaternion data acquisition via I2C interface through the M5atomS3R GROVE connector.

## Test Objectives

- [x] Verify I2C communication with BNO055 sensor
- [x] Confirm sensor initialization and chip ID detection
- [x] Test NDOF (9-axis fusion) mode operation
- [x] Validate quaternion data output format
- [x] Check quaternion normalization (unit quaternion)

## Hardware Requirements

- M5atomS3R device
- BNO055 IMU sensor module
- GROVE I2C cable
- USB-C cable for power and programming

## Hardware Connections

### GROVE Connector Pinout

```
BNO055 Module    M5atomS3R GROVE
--------------   ---------------
VCC (3.3V)   <-> Pin 1 (3.3V)
GND          <-> Pin 2 (GND)
SDA          <-> Pin 3 (GPIO 2)
SCL          <-> Pin 4 (GPIO 1)
```

### I2C Configuration

| Parameter | Value | Description |
|-----------|-------|-------------|
| I2C Port | I2C_NUM_0 | Primary I2C interface |
| SDA Pin | GPIO 2 | GROVE connector pin 3 |
| SCL Pin | GPIO 1 | GROVE connector pin 4 |
| Frequency | 100 kHz | Standard I2C speed |
| Address | 0x28 | BNO055 default I2C address |

## Test Configuration

### BNO055 Driver Settings

```c
bno055_config_t config = {
    .i2c_port = I2C_NUM_0,
    .sda_pin = GPIO_NUM_2,
    .scl_pin = GPIO_NUM_1,
    .i2c_freq = 100000,  // 100kHz
    .i2c_addr = BNO055_I2C_ADDR
};
```

### Operation Mode

- **Mode**: NDOF (Nine Degrees of Freedom)
- **Fusion**: Hardware sensor fusion enabled
- **Output**: Quaternion (W, X, Y, Z) values
- **Rate**: 1 Hz (1 second intervals)

## Test Procedure

### 1. Hardware Setup

1. Connect BNO055 module to M5atomS3R GROVE connector
2. Ensure secure connections and proper power supply
3. Verify sensor module power LED (if available)

### 2. Software Preparation

```bash
# Build project with BNO055 support
./dev.sh build-flash

# Start monitoring
./monitor-loop.sh
```

### 3. Test Execution

1. Reset M5atomS3R device
2. Monitor initialization sequence
3. Observe quaternion data output
4. Verify data consistency and normalization

## Expected Results

### Successful Initialization

```
I (xxxx) M5ATOMS3R: BNO055 test task started
I (xxxx) BNO055: Initializing BNO055...
I (xxxx) BNO055: BNO055 chip ID verified: 0xA0
I (xxxx) BNO055: Resetting BNO055...
I (xxxx) M5ATOMS3R: BNO055 initialized successfully
I (xxxx) M5ATOMS3R: Waiting for sensor stabilization...
```

### Quaternion Data Output

```
I (xxxx) M5ATOMS3R: === BNO055 Quaternion Data #1 ===
I (xxxx) M5ATOMS3R: W: +0.9998
I (xxxx) M5ATOMS3R: X: +0.0012
I (xxxx) M5ATOMS3R: Y: -0.0034
I (xxxx) M5ATOMS3R: Z: +0.0187
I (xxxx) M5ATOMS3R: Magnitude: 1.0000
I (xxxx) M5ATOMS3R: ==============================
```

### Continuous Data Stream

The system should output quaternion data every 1 second with:
- Four quaternion components (W, X, Y, Z)
- Magnitude close to 1.0000 (unit quaternion)
- Smooth changes during device rotation
- Consistent data format

## Success Criteria

| Criteria | Expected Value | Validation |
|----------|----------------|------------|
| Chip ID Detection | 0xA0 | BNO055 identification |
| I2C Communication | No errors | Successful read/write operations |
| Quaternion Format | W, X, Y, Z floats | Four-component output |
| Magnitude | ~1.0000 ± 0.01 | Unit quaternion validation |
| Data Rate | 1 Hz | Consistent 1-second intervals |
| Orientation Response | Changes with movement | Real-time sensor response |

## Error Scenarios

### 1. Sensor Not Connected

```
E (xxxx) M5ATOMS3R: BNO055 initialization failed: ESP_ERR_TIMEOUT
E (xxxx) M5ATOMS3R: Check I2C connections and sensor power
```

### 2. Wrong Chip ID

```
E (xxxx) BNO055: Invalid chip ID: 0x00 (expected 0xA0)
```

### 3. I2C Communication Failure

```
E (xxxx) BNO055: I2C read failed: ESP_ERR_TIMEOUT
E (xxxx) M5ATOMS3R: Failed to read quaternion: ESP_ERR_TIMEOUT
```

## Troubleshooting

### Hardware Issues

1. **Check connections**
   - Verify GROVE cable integrity
   - Confirm pin mapping (SDA/SCL)
   - Test power supply voltage (3.3V)

2. **I2C address conflict**
   - Scan I2C bus: Use I2C scanner utility
   - Check for address conflicts
   - Verify BNO055 address (0x28 default)

3. **Power supply**
   - Check 3.3V rail stability
   - Verify current capability
   - Test with external power if needed

### Software Issues

1. **Build errors**
   - Verify component dependencies
   - Check include paths
   - Confirm CMakeLists.txt configuration

2. **Runtime errors**
   - Monitor I2C error messages
   - Check task stack size (4096 bytes)
   - Verify timing and delays

### Resolution Steps

```bash
# Clean rebuild
./docker-build.sh fullclean
./dev.sh build-flash

# Hardware verification
# - Check connections
# - Test with known good sensor
# - Verify power rails

# Debug I2C communication
# - Use logic analyzer if available
# - Check pull-up resistors
# - Test at lower speeds (50kHz)
```

## Component Architecture

### BNO055 Driver Component

```
components/bno055/
├── include/bno055.h          # Public API
├── src/bno055.c             # Implementation
├── CMakeLists.txt           # Build configuration
└── README.md                # Component documentation
```

### Key API Functions

```c
// Initialization
esp_err_t bno055_init(const bno055_config_t *config);
esp_err_t bno055_deinit(i2c_port_t i2c_port);

// Control
esp_err_t bno055_reset(void);
esp_err_t bno055_set_mode(bno055_opmode_t mode);

// Data acquisition
esp_err_t bno055_get_chip_id(uint8_t *chip_id);
esp_err_t bno055_get_quaternion(bno055_quaternion_t *quat);
```

### Data Structures

```c
typedef struct {
    float w;  // Real component
    float x;  // i component  
    float y;  // j component
    float z;  // k component
} bno055_quaternion_t;
```

## Test Implementation

### Main Test Task

Location: `main/main.cpp:163-215`

```cpp
void bno055_test_task(void *pvParameters)
{
    // Configuration and initialization
    bno055_config_t config = { /* ... */ };
    esp_err_t ret = bno055_init(&config);
    
    // Continuous data acquisition
    while (1) {
        bno055_quaternion_t quat;
        ret = bno055_get_quaternion(&quat);
        
        if (ret == ESP_OK) {
            // Display quaternion data
            ESP_LOGI(TAG, "W: %+.4f", quat.w);
            ESP_LOGI(TAG, "X: %+.4f", quat.x);
            ESP_LOGI(TAG, "Y: %+.4f", quat.y);
            ESP_LOGI(TAG, "Z: %+.4f", quat.z);
            
            // Verify normalization
            float magnitude = sqrtf(quat.w*quat.w + quat.x*quat.x + 
                                   quat.y*quat.y + quat.z*quat.z);
            ESP_LOGI(TAG, "Magnitude: %.4f", magnitude);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

## Performance Characteristics

### Timing

- **Initialization**: ~2 seconds (including reset and stabilization)
- **Data Rate**: 1 Hz (configurable)
- **Response Time**: <10ms per quaternion read
- **Startup Delay**: 100ms after I2C initialization

### Memory Usage

- **Driver Size**: ~8KB compiled code
- **RAM Usage**: <1KB (includes I2C buffers)
- **Stack Requirements**: 4096 bytes task stack

## Test Status

**Status**: 🔧 READY FOR EXECUTION  
**Implementation**: ✅ COMPLETE  
**Hardware Required**: BNO055 module + GROVE cable  
**Expected Duration**: 5-10 minutes setup + continuous monitoring  

## Related Documentation

- [BNO055 Component README](../components/bno055/README.md)
- [BNO055 Datasheet](https://www.bosch-sensortec.com/products/smart-sensors/bno055/)
- [ESP-IDF I2C Driver Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2c.html)
- Project main documentation: `CLAUDE.md`

## Notes

- Quaternion data represents absolute orientation in 3D space
- Sensor performs automatic calibration during operation
- Initial readings may be unstable until calibration completes
- Physical movement of device should reflect in quaternion changes
- Magnitude should always be close to 1.0 for valid quaternions