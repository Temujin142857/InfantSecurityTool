#ifndef MPU60501_H
#define MPU60501_H

#include <Wire.h>


void mpu_init(TwoWire *wire);

void readMotion(float *motion);

#endif