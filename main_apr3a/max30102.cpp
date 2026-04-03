#include "max30102.h"
#include <DFRobot_MAX30102.h>


DFRobot_MAX30102 particleSensor;

void max30102_init(){
   while (!particleSensor.begin()) {
    Serial.println("MAX30102 was not found");
    delay(1000);
  }

   /*!
   *@brief Use macro definition to configure sensor
   *@param ledBrightness LED brightness, default value: 0x1F（6.4mA), Range: 0~255（0=Off, 255=50mA）
   *@param sampleAverage Average multiple samples then draw once, reduce data throughput, default 4 samples average
   *@param ledMode LED mode, default to use red light and IR at the same time 
   *@param sampleRate Sampling rate, default 400 samples every second 
   *@param pulseWidth Pulse width: the longer the pulse width, the wider the detection range. Default to be Max range
   *@param adcRange Measurement Range, default 4096 (nA), 15.63(pA) per LSB
   */
  particleSensor.sensorConfiguration(/*ledBrightness=*/0x1F, /*sampleAverage=*/SAMPLEAVG_4, \
                                  /*ledMode=*/MODE_MULTILED, /*sampleRate=*/SAMPLERATE_400, \
                                  /*pulseWidth=*/PULSEWIDTH_411, /*adcRange=*/ADCRANGE_4096);
}

void readHeartRateAndBloodOxygen(int32_t *SPO2,int8_t *SPO2Valid,int32_t *heartRate,int8_t *heartRateValid){
  particleSensor.heartrateAndOxygenSaturation(SPO2,SPO2Valid,heartRate,heartRateValid);
}
