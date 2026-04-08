#include "max30102.h"
#include <Wire.h>
#include <MAX30105.h>
#include "heartRate.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;
static bool max_ready = false;

// Buffers required for SpO2 algorithm
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
uint16_t irBuffer[100];
uint16_t redBuffer[100];
#else
uint32_t irBuffer[100];
uint32_t redBuffer[100];
#endif

void max30102_init(TwoWire *wire) {
    Serial.println("Initializing MAX30102...");

    if (!particleSensor.begin(*wire, 400000, 0x57)) {
        Serial.println("MAX30102 was not found. Check wiring.");
        max_ready = false;
        return;
    }

    byte ledBrightness = 60;   // un peu plus fort
    byte sampleAverage = 4;
    byte ledMode = 2;          // Red + IR
    int sampleRate = 100;
    int pulseWidth = 411;
    int adcRange = 16384;

    particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
    particleSensor.setPulseAmplitudeRed(0x24);
    particleSensor.setPulseAmplitudeIR(0x24);

    Serial.println("MAX30102 initialized");
    max_ready = true;
}

void readHeartRateAndBloodOxygen(int32_t *spo2, int8_t *spo2_valid, int32_t *heart_rate, int8_t *hr_valid) {
    if (!max_ready) {
        Serial.println("MAX not ready");
        *heart_rate = 0;
        *hr_valid = 0;
        *spo2 = 0;
        *spo2_valid = 0;
        return;
    }

    // collect 100 samples
    for (int i = 0; i < 100; i++) {
        while (particleSensor.available() == false) {
            particleSensor.check();
        }

        redBuffer[i] = particleSensor.getRed();
        irBuffer[i] = particleSensor.getIR();
        particleSensor.nextSample();
    }

    Serial.print("IR sample: ");
    Serial.print(irBuffer[99]);
    Serial.print(" | RED sample: ");
    Serial.println(redBuffer[99]);

    // finger detection
    if (irBuffer[99] < 5000) {
        Serial.println("Place finger properly on sensor");
        *heart_rate = 0;
        *hr_valid = 0;
        *spo2 = 0;
        *spo2_valid = 0;
        return;
    }

    // Use Maxim algorithm
    maxim_heart_rate_and_oxygen_saturation(
        irBuffer,
        100,
        redBuffer,
        spo2,
        spo2_valid,
        heart_rate,
        hr_valid
    );

    Serial.print("Heart Rate: ");
    Serial.print(*heart_rate);
    Serial.print(" | valid: ");
    Serial.println(*hr_valid);

    Serial.print("SpO2: ");
    Serial.print(*spo2);
    Serial.print(" | valid: ");
    Serial.println(*spo2_valid);
}