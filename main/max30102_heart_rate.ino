#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

long lastBeat = 0;
float bpm = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(18, 19);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 non detecte.");
    while (1);
  }

  particleSensor.setup(
    60,   // brightness
    4,    // sampleAverage
    2,    // ledMode
    100,  // sampleRate
    411,  // pulseWidth
    4096  // adcRange
  );

  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);

  Serial.println("Pose ton doigt sur le capteur...");
}

void loop() {
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    bpm = 60.0 / (delta / 1000.0);

    Serial.print("Battement detecte ! BPM: ");
    Serial.println(bpm);
  }

  Serial.print("IR = ");
  Serial.println(irValue);

  delay(20);
}
