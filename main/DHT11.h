#ifndef DHT11_H
#define DHT11_H

#include "driver/gpio.h"
#include "esp_err.h"

typedef struct {
    int temperature;
    int humidity;
} dht11_data_t;

esp_err_t dht11_read(gpio_num_t pin, dht11_data_t *out);

#endif