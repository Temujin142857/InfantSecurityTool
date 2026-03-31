#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include "driver/i2c.h"

// Default I2C address
#define MPU6050_ADDR 0x68

// Registers
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B

typedef struct {
    float ax, ay, az;
    float gx, gy, gz;
} mpu6050_data_t;

esp_err_t mpu6050_init(void);
esp_err_t mpu6050_read(mpu6050_data_t *data);
float motion_delta(mpu6050_data_t *d);

#endif