#ifndef MAX30102_H
#define MAX30102_H

#include <Wire.h>
#include <stdint.h>

void max30102_init(TwoWire *wire);
void readHeartRateAndBloodOxygen(int32_t *spo2, int8_t *spo2_valid, int32_t *heart_rate, int8_t *hr_valid);

#endif