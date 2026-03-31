#ifndef I2C_H
#define I2C_H
#include "esp_err.h"

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

esp_err_t i2c_master_init(void);

void i2c_scan();


#endif