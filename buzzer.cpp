#include "buzzer.h"
#include <Arduino.h>
#define BUZZER_PIN 15

void buzzer_init() {
    pinMode(BUZZER_PIN, OUTPUT);
}

void buzzer_on() {
    digitalWrite(BUZZER_PIN, HIGH);
}

void buzzer_off() {
    digitalWrite(BUZZER_PIN, LOW);
}

void buzzer_toggle() {
    static bool state = false;
    state = !state;
    digitalWrite(BUZZER_PIN, state);
}

void buzzer_beep() {
    buzzer_on();
    delay(200);
    buzzer_off();
    delay(200);
}