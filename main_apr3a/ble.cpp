#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "ble.h"

static BLECharacteristic *pHeartRateChar;
static BLECharacteristic *pTempChar;
static BLECharacteristic *pSmokeChar;
static BLECharacteristic *pSpO2Char;
static BLECharacteristic *pMotionChar;
static BLECharacteristic *pHumidityChar;
static BLECharacteristic *pBrightnessChar;

#define SERVICE_UUID        "eafeeb4a-98e9-41ef-b304-1c493eab2e84"
#define HEART_RATE_UUID     "3497534f-cecd-4def-94f8-1bc40895143d"
#define ETEMPURUATURE_UUID  "6e4bcb0b-6423-424a-a684-b88a1879ab1f"
#define ITEMPURUATURE_UUID  "5db0b8c4-b303-408e-b685-0bbb9fb5e682"
#define SMOKE_UUID          "8fa9131c-3416-4daf-8c28-5a7febb3edba"
#define SPO2_UUID           "5b462ef2-f98d-414e-90d3-a051af485dee"
#define MOTION_UUID         "085730ce-db2a-459a-9e8d-69bbd744f063"
#define HUMIDITY_UUID       "12de0149-ee0a-45dd-9db6-167d50929828"
#define BRIGHTNESS_UUID     "9a737158-b0fc-4845-84e3-5a40afd05751"

void ble_init(const char *device_name) {
    BLEDevice::init(device_name);
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);

    pHeartRateChar = pService->createCharacteristic(HEART_RATE_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pTempChar      = pService->createCharacteristic(TEMP_UUID,      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pSmokeChar     = pService->createCharacteristic(SMOKE_UUID,     BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pSpO2Char      = pService->createCharacteristic(SPO2_UUID,      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pMotionChar    = pService->createCharacteristic(MOTION_UUID,    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pHumidityChar  = pService->createCharacteristic(HUMIDITY_UUID,  BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pBrightnessChar= pService->createCharacteristic(BRIGHTNESS_UUID,BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();
}

// Heart Rate (bpm)
void ble_set_heart_rate(int bpm) {
    char buffer[8];
    sprintf(buffer, "%d", bpm);
    pHeartRateChar->setValue(buffer);
    pHeartRateChar->notify();
}

// Internal Temperature (°C)
void ble_set_itemperature(float temp) {
    char buffer[8];
    sprintf(buffer, "%.1f", temp);
    pTempChar->setValue(buffer);
    pTempChar->notify();
}

// External Temperature (°C)
void ble_set_etemperature(float temp) {
    char buffer[8];
    sprintf(buffer, "%.1f", temp);
    pTempChar->setValue(buffer);
    pTempChar->notify();
}

// Smoke Level (ppm or 0/1)
void ble_set_smoke(int smoke) {
    char buffer[8];
    sprintf(buffer, "%d", smoke);
    pSmokeChar->setValue(buffer);
    pSmokeChar->notify();
}

// Blood Oxygen (SpO2 %)
void ble_set_spo2(int spo2) {
    char buffer[8];
    sprintf(buffer, "%d", spo2);
    pSpO2Char->setValue(buffer);
    pSpO2Char->notify();
}

// Time Since Last Motion (seconds)
void ble_set_motion_time(int seconds) {
    char buffer[8];
    sprintf(buffer, "%d", seconds);
    pMotionChar->setValue(buffer);
    pMotionChar->notify();
}

// Humidity (%)
void ble_set_humidity(float humidity) {
    char buffer[8];
    sprintf(buffer, "%.1f", humidity);
    pHumidityChar->setValue(buffer);
    pHumidityChar->notify();
}

// Brightness (lux)
void ble_set_brightness(int brightness) {
    char buffer[8];
    sprintf(buffer, "%d", brightness);
    pBrightnessChar->setValue(buffer);
    pBrightnessChar->notify();
}
