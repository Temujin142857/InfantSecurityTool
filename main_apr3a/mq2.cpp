#include "mq2.h"
#include <Arduino.h>
#define MQ2_PIN 34  // ADC pin


void smoke_init() {
    analogReadResolution(12); // ESP32 = 0–4095
}

int read_smoke_raw() {
    return analogRead(MQ2_PIN);
}