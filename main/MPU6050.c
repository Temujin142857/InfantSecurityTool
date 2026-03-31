#include "mpu6050.h"
#include <math.h>
#include "I2C.h"

#define I2C_PORT I2C_MASTER_NUM

static esp_err_t write_byte(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    return i2c_master_write_to_device(I2C_PORT, MPU6050_ADDR, buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t read_bytes(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_PORT, MPU6050_ADDR, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

esp_err_t mpu6050_init(void)
{
    return write_byte(MPU6050_PWR_MGMT_1, 0x00);
}


esp_err_t mpu6050_read(mpu6050_data_t *data)
{
    uint8_t raw[14];

    esp_err_t ret = read_bytes(MPU6050_ACCEL_XOUT_H, raw, 14);
    if (ret != ESP_OK) return ret;

    int16_t ax = (raw[0] << 8) | raw[1];
    int16_t ay = (raw[2] << 8) | raw[3];
    int16_t az = (raw[4] << 8) | raw[5];

    int16_t gx = (raw[8] << 8) | raw[9];
    int16_t gy = (raw[10] << 8) | raw[11];
    int16_t gz = (raw[12] << 8) | raw[13];

    // Convert to physical units
    data->ax = ax / 16384.0f; // g
    data->ay = ay / 16384.0f;
    data->az = az / 16384.0f;

    data->gx = gx / 131.0f;   // deg/s
    data->gy = gy / 131.0f;
    data->gz = gz / 131.0f;

    return ESP_OK;
}


float motion_delta(mpu6050_data_t *d)
{
    static float last = 0;
    float current = fabs(d->ax) + fabs(d->ay) + fabs(d->az);
    float delta = fabs(current - last);
    last = current;
    return delta;
}

/*
uint8_t motionCheck_mpu(mpu6050_data_t *d)
{
    float motion = motion_delta(d);

    if (motion < 0.2f) {
        return 2; // No movement (possible issue)
    }
    else if (motion < 0.5f) {
        return 1; // Low movement
    }
    return 0; // Normal
}
*/
