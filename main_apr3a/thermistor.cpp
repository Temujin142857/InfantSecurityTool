#include "thermistor.h"
#include <Arduino.h>
#include <math.h>

#define THERMISTOR_PIN 34   // GPIO34 (ADC1, safe pin)
#define SERIES_RESISTOR 10000  // 10k resistor
#define NOMINAL_RESISTANCE 10000  // 10k thermistor at 25°C
#define NOMINAL_TEMPERATURE 25    // °C
#define BETA_COEFFICIENT 3950     // check your thermistor datasheet


void readITempurature(float *temp) {
  int adcValue = analogRead(THERMISTOR_PIN);

  // Convert ADC value to voltage
  float voltage = adcValue * (3.3 / 4095.0);

  // Calculate thermistor resistance
  float resistance = SERIES_RESISTOR * (3.3 / voltage - 1);

  // Beta formula
  float steinhart;
  steinhart = resistance / NOMINAL_RESISTANCE;     
  steinhart = log(steinhart);                     
  steinhart /= BETA_COEFFICIENT;                 
  steinhart += 1.0 / (NOMINAL_TEMPERATURE + 273.15); 
  steinhart = 1.0 / steinhart;                   
  steinhart -= 273.15;    

  *temp= steinhart;              
}