#include "led.h"
#include <Arduino.h>
#define LED_PIN 2  

void led_init() {
    pinMode(LED_PIN, OUTPUT);
}

void led_on() {
    digitalWrite(LED_PIN, HIGH);
}

void led_off() {
    digitalWrite(LED_PIN, LOW);
}