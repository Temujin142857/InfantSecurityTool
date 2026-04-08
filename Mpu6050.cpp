#include "Mpu6050.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

Adafruit_MPU6050 mpu;
static bool mpu_ready = false;

void mpu_init(TwoWire *wire) {
    delay(500);

    Serial.println("Initializing MPU6050...");

    if (mpu.begin(0x68, wire)) {
        Serial.println("MPU6050 found at 0x68");
        mpu_ready = true;
    }
    else if (mpu.begin(0x69, wire)) {
        Serial.println("MPU6050 found at 0x69");
        mpu_ready = true;
    }
    else {
        Serial.println("ERROR: MPU6050 not found at 0x68 or 0x69");
        mpu_ready = false;
        return;
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    Serial.println("MPU6050 ready!");
}

void readMotion(float *motion) {
    if (!mpu_ready) {
        Serial.println("MPU not ready");
        *motion = 0;
        return;
    }

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    *motion = sqrt(
        a.acceleration.x * a.acceleration.x +
        a.acceleration.y * a.acceleration.y +
        a.acceleration.z * a.acceleration.z
    );
}