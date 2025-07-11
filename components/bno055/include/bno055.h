#ifndef BNO055_H
#define BNO055_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// BNO055 I2C address
#define BNO055_I2C_ADDR         0x28

// BNO055 Register addresses
#define BNO055_CHIP_ID_ADDR     0x00
#define BNO055_PAGE_ID_ADDR     0x07
#define BNO055_ACCEL_REV_ID_ADDR 0x01
#define BNO055_MAG_REV_ID_ADDR  0x02
#define BNO055_GYRO_REV_ID_ADDR 0x03
#define BNO055_SW_REV_ID_LSB_ADDR 0x04
#define BNO055_SW_REV_ID_MSB_ADDR 0x05
#define BNO055_BL_REV_ID_ADDR   0x06

// Mode registers
#define BNO055_OPR_MODE_ADDR    0x3D
#define BNO055_PWR_MODE_ADDR    0x3E
#define BNO055_SYS_TRIGGER_ADDR 0x3F

// Quaternion data registers
#define BNO055_QUATERNION_DATA_W_LSB_ADDR 0x20
#define BNO055_QUATERNION_DATA_W_MSB_ADDR 0x21
#define BNO055_QUATERNION_DATA_X_LSB_ADDR 0x22
#define BNO055_QUATERNION_DATA_X_MSB_ADDR 0x23
#define BNO055_QUATERNION_DATA_Y_LSB_ADDR 0x24
#define BNO055_QUATERNION_DATA_Y_MSB_ADDR 0x25
#define BNO055_QUATERNION_DATA_Z_LSB_ADDR 0x26
#define BNO055_QUATERNION_DATA_Z_MSB_ADDR 0x27

// Operation modes
typedef enum {
    BNO055_OPERATION_MODE_CONFIG        = 0x00,
    BNO055_OPERATION_MODE_ACCONLY       = 0x01,
    BNO055_OPERATION_MODE_MAGONLY       = 0x02,
    BNO055_OPERATION_MODE_GYRONLY       = 0x03,
    BNO055_OPERATION_MODE_ACCMAG        = 0x04,
    BNO055_OPERATION_MODE_ACCGYRO       = 0x05,
    BNO055_OPERATION_MODE_MAGGYRO       = 0x06,
    BNO055_OPERATION_MODE_AMG           = 0x07,
    BNO055_OPERATION_MODE_IMUPLUS       = 0x08,
    BNO055_OPERATION_MODE_COMPASS       = 0x09,
    BNO055_OPERATION_MODE_M4G           = 0x0A,
    BNO055_OPERATION_MODE_NDOF_FMC_OFF  = 0x0B,
    BNO055_OPERATION_MODE_NDOF          = 0x0C
} bno055_opmode_t;

// Power modes
typedef enum {
    BNO055_POWER_MODE_NORMAL = 0x00,
    BNO055_POWER_MODE_LOWPOWER = 0x01,
    BNO055_POWER_MODE_SUSPEND = 0x02
} bno055_powermode_t;

// Quaternion data structure
typedef struct {
    float w;
    float x;
    float y;
    float z;
} bno055_quaternion_t;

// BNO055 configuration structure
typedef struct {
    i2c_port_t i2c_port;
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    uint32_t i2c_freq;
    uint8_t i2c_addr;
} bno055_config_t;

// Function prototypes
esp_err_t bno055_init(const bno055_config_t *config);
esp_err_t bno055_deinit(i2c_port_t i2c_port);
esp_err_t bno055_reset(void);
esp_err_t bno055_set_mode(bno055_opmode_t mode);
esp_err_t bno055_set_power_mode(bno055_powermode_t power_mode);
esp_err_t bno055_get_chip_id(uint8_t *chip_id);
esp_err_t bno055_get_quaternion(bno055_quaternion_t *quat);
bool bno055_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif // BNO055_H