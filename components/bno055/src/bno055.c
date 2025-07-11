#include "bno055.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "BNO055";

static i2c_port_t bno055_i2c_port = I2C_NUM_0;
static uint8_t bno055_i2c_addr = BNO055_I2C_ADDR;
static bool bno055_initialized = false;

// Private functions
static esp_err_t bno055_write_reg(uint8_t reg_addr, uint8_t data);
static esp_err_t bno055_read_reg(uint8_t reg_addr, uint8_t *data);
static esp_err_t bno055_read_burst(uint8_t reg_addr, uint8_t *data, size_t len);

static esp_err_t bno055_write_reg(uint8_t reg_addr, uint8_t data)
{
    if (!bno055_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (bno055_i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(bno055_i2c_port, cmd, 100 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C write failed: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

static esp_err_t bno055_read_reg(uint8_t reg_addr, uint8_t *data)
{
    if (!bno055_initialized || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (bno055_i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (bno055_i2c_addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(bno055_i2c_port, cmd, 100 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

static esp_err_t bno055_read_burst(uint8_t reg_addr, uint8_t *data, size_t len)
{
    if (!bno055_initialized || data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (bno055_i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (bno055_i2c_addr << 1) | I2C_MASTER_READ, true);
    
    for (size_t i = 0; i < len - 1; i++) {
        i2c_master_read_byte(cmd, &data[i], I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, &data[len - 1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(bno055_i2c_port, cmd, 200 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C burst read failed: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t bno055_init(const bno055_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing BNO055...");

    // Store configuration
    bno055_i2c_port = config->i2c_port;
    bno055_i2c_addr = config->i2c_addr;

    // Initialize I2C
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_pin,
        .scl_io_num = config->scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = config->i2c_freq
    };

    esp_err_t ret = i2c_param_config(bno055_i2c_port, &i2c_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(bno055_i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    bno055_initialized = true;

    // Wait for BNO055 to be ready
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Check chip ID
    uint8_t chip_id;
    ret = bno055_get_chip_id(&chip_id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read chip ID");
        bno055_deinit(bno055_i2c_port);
        return ret;
    }

    if (chip_id != 0xA0) {
        ESP_LOGE(TAG, "Invalid chip ID: 0x%02X (expected 0xA0)", chip_id);
        bno055_deinit(bno055_i2c_port);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "BNO055 chip ID verified: 0x%02X", chip_id);

    // Reset device
    ret = bno055_reset();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BNO055 reset failed");
        bno055_deinit(bno055_i2c_port);
        return ret;
    }

    // Set to NDOF mode for quaternion output
    ret = bno055_set_mode(BNO055_OPERATION_MODE_NDOF);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set NDOF mode");
        bno055_deinit(bno055_i2c_port);
        return ret;
    }

    ESP_LOGI(TAG, "BNO055 initialized successfully");
    return ESP_OK;
}

esp_err_t bno055_deinit(i2c_port_t i2c_port)
{
    esp_err_t ret = i2c_driver_delete(i2c_port);
    bno055_initialized = false;
    return ret;
}

esp_err_t bno055_reset(void)
{
    ESP_LOGI(TAG, "Resetting BNO055...");
    
    // Set to config mode first
    esp_err_t ret = bno055_set_mode(BNO055_OPERATION_MODE_CONFIG);
    if (ret != ESP_OK) {
        return ret;
    }

    // Trigger reset
    ret = bno055_write_reg(BNO055_SYS_TRIGGER_ADDR, 0x20);
    if (ret != ESP_OK) {
        return ret;
    }

    // Wait for reset to complete
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    return ESP_OK;
}

esp_err_t bno055_set_mode(bno055_opmode_t mode)
{
    ESP_LOGD(TAG, "Setting operation mode to 0x%02X", mode);
    
    esp_err_t ret = bno055_write_reg(BNO055_OPR_MODE_ADDR, mode);
    if (ret != ESP_OK) {
        return ret;
    }

    // Wait for mode change
    if (mode == BNO055_OPERATION_MODE_CONFIG) {
        vTaskDelay(19 / portTICK_PERIOD_MS);
    } else {
        vTaskDelay(7 / portTICK_PERIOD_MS);
    }

    return ESP_OK;
}

esp_err_t bno055_set_power_mode(bno055_powermode_t power_mode)
{
    return bno055_write_reg(BNO055_PWR_MODE_ADDR, power_mode);
}

esp_err_t bno055_get_chip_id(uint8_t *chip_id)
{
    return bno055_read_reg(BNO055_CHIP_ID_ADDR, chip_id);
}

esp_err_t bno055_get_quaternion(bno055_quaternion_t *quat)
{
    if (quat == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buffer[8];
    esp_err_t ret = bno055_read_burst(BNO055_QUATERNION_DATA_W_LSB_ADDR, buffer, 8);
    if (ret != ESP_OK) {
        return ret;
    }

    // Convert raw data to quaternion values
    // BNO055 quaternion format: 1 unit = 1/16384
    int16_t w = (int16_t)((buffer[1] << 8) | buffer[0]);
    int16_t x = (int16_t)((buffer[3] << 8) | buffer[2]);
    int16_t y = (int16_t)((buffer[5] << 8) | buffer[4]);
    int16_t z = (int16_t)((buffer[7] << 8) | buffer[6]);

    const float scale = 1.0f / 16384.0f;
    quat->w = w * scale;
    quat->x = x * scale;
    quat->y = y * scale;
    quat->z = z * scale;

    return ESP_OK;
}

bool bno055_is_initialized(void)
{
    return bno055_initialized;
}