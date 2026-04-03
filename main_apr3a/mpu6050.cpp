#include "mpu6050.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;


float last = 0;


void mpu_init(){
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
}

void readMotion(float *motion){
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float current = abs(a.acceleration.x) +
                  abs(a.acceleration.y) +
                  abs(a.acceleration.z);

  static float last = 0;
  float delta = abs(current - last);
  last = current;

  *motion = delta;
}


 

