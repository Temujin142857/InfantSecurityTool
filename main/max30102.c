#include "max30102.h"      
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>


#define MAX30102_ADDR 0x57

static esp_err_t write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_write_to_device(I2C_NUM_0, MAX30102_ADDR, data, 2, 100);
}

esp_err_t max30102_init(void)
{
    // Reset
    write_reg(0x09, 0x40);
    vTaskDelay(pdMS_TO_TICKS(100));

    // FIFO config
    write_reg(0x08, 0x0F);

    // Mode: SpO2
    write_reg(0x09, 0x03);

    // Sample rate + pulse width
    write_reg(0x0A, 0x27); // ~100Hz, 411us

    // LED currents
    write_reg(0x0C, 0x24); // RED
    write_reg(0x0D, 0x24); // IR

    return ESP_OK;
}

uint8_t max30102_get_fifo_samples()
{
    uint8_t wr_ptr, rd_ptr;

    i2c_master_write_read_device(I2C_NUM_0, MAX30102_ADDR,
        (uint8_t[]){0x04}, 1, &wr_ptr, 1, 100);

    i2c_master_write_read_device(I2C_NUM_0, MAX30102_ADDR,
        (uint8_t[]){0x06}, 1, &rd_ptr, 1, 100);

    return (wr_ptr - rd_ptr) & 0x1F;
}

int max30102_read_fifo(max_sample_t *samples, int max_samples)
{
    int available = max30102_get_fifo_samples();
    int to_read = (available < max_samples) ? available : max_samples;

    uint8_t reg = 0x07;
    uint8_t data[6 * to_read];

    if (to_read == 0)
        return 0;

    i2c_master_write_read_device(
        I2C_NUM_0,
        MAX30102_ADDR,
        &reg, 1,
        data, 6 * to_read,
        100
    );

    for (int i = 0; i < to_read; i++)
    {
        samples[i].ir =
            ((uint32_t)data[i*6] << 16 |
             (uint32_t)data[i*6+1] << 8 |
             (uint32_t)data[i*6+2]) & 0x03FFFF;

        samples[i].red =
            ((uint32_t)data[i*6+3] << 16 |
             (uint32_t)data[i*6+4] << 8 |
             (uint32_t)data[i*6+5]) & 0x03FFFF;
    }

    return to_read;
}