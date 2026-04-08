#ifndef MPU6050_H
#define MPU6050_H

#include <Arduino.h>
#include <Wire.h>

void mpu_init(TwoWire *wire);
void readMotion(float *motion);

#endif