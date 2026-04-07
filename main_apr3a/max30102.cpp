#include "max30102.h"
#include <Wire.h>
#include <MAX30105.h>

MAX30105 particleSensor;

void max30102_init(TwoWire *wire) {
  if (!particleSensor.begin(*wire, 400000, 0x57)) {
    Serial.println("MAX30102 was not found. Please check wiring/power.");
    
  } else{

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);

  Serial.println("MAX30102 initialized");
  }


}

void readHeartRateAndBloodOxygen(int32_t *SPO2, int8_t *SPO2Valid, int32_t *heartRate, int8_t *heartRateValid) {
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  *heartRate = 0;
  *heartRateValid = 0;
  *SPO2 = 0;
  *SPO2Valid = 0;

  Serial.print("IR: ");
  Serial.print(irValue);
  Serial.print(" | RED: ");
  Serial.println(redValue);
}