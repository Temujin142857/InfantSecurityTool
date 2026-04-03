#ifndef MAX30102_H
#define MAX30102_H
#include <unistd.h>

void max30102_init();

void readHeartRateAndBloodOxygen(int32_t *SPO2,int8_t *SPO2Valid,int32_t *heartRate,int8_t *heartRateValid);

#endif