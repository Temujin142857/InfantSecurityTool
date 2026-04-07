#include "Mpu6050.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

void mpu_init(TwoWire *Wire) {
  delay(100);

  if (!mpu.begin(0x68, Wire)) {
    Serial.println("Failed to find MPU6050 chip");
    
  }else{
    Serial.println("MPU6050 Found!");

 }
}

void readMotion(float *motion) {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float current = fabs(a.acceleration.x) + fabs(a.acceleration.y) + fabs(a.acceleration.z);
  static float last = 0;
  float delta = fabs(current - last);
  last = current;

  *motion = delta;
}