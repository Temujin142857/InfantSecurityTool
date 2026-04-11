#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "ble.h"

static BLECharacteristic *pAllSensorsChar;

static BLECharacteristic *pHeartRateChar;
static BLECharacteristic *pTempChar;
static BLECharacteristic *pSmokeChar;
static BLECharacteristic *pSpO2Char;
static BLECharacteristic *pMotionChar;
static BLECharacteristic *pHumidityChar;
static BLECharacteristic *pBrightnessChar;
static BLEServer *pServer = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;


# define ALL_SENSORS_UUID "10b80e40-fb4c-454a-ba7c-0cc397825fbb"

#define SERVICE_UUID        "eafeeb4a-98e9-41ef-b304-1c493eab2e84"
#define HEART_RATE_UUID     "3497534f-cecd-4def-94f8-1bc40895143d"
#define ETEMPURUATURE_UUID  "6e4bcb0b-6423-424a-a684-b88a1879ab1f"
#define ITEMPURUATURE_UUID  "5db0b8c4-b303-408e-b685-0bbb9fb5e682"
#define SMOKE_UUID          "8fa9131c-3416-4daf-8c28-5a7febb3edba"
#define SPO2_UUID           "5b462ef2-f98d-414e-90d3-a051af485dee"
#define MOTION_UUID         "085730ce-db2a-459a-9e8d-69bbd744f063"
#define HUMIDITY_UUID       "12de0149-ee0a-45dd-9db6-167d50929828"
#define BRIGHTNESS_UUID     "9a737158-b0fc-4845-84e3-5a40afd05751"

class MyServerCallbacks : public BLEServerCallbacks {
      void onConnect(BLEServer* pServer) {
          deviceConnected = true;
          Serial.println("BLE client connected");
      }
      void onDisconnect(BLEServer* pServer) {
          deviceConnected = false;
      }
  };
  
void ble_init(const char *device_name) {
    BLEDevice::init(device_name);
    pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pServer->setCallbacks(new MyServerCallbacks());

    pAllSensorsChar = pService->createCharacteristic(
        ALL_SENSORS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pAllSensorsChar->addDescriptor(new BLE2902());

/*
    pHeartRateChar = pService->createCharacteristic(HEART_RATE_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pTempChar      = pService->createCharacteristic(TEMP_UUID,      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pSmokeChar     = pService->createCharacteristic(SMOKE_UUID,     BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pSpO2Char      = pService->createCharacteristic(SPO2_UUID,      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pMotionChar    = pService->createCharacteristic(MOTION_UUID,    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pHumidityChar  = pService->createCharacteristic(HUMIDITY_UUID,  BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pBrightnessChar= pService->createCharacteristic(BRIGHTNESS_UUID,BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
*/
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();
}

void ble_set_all_sensors(char id, float valf, int vali) {
      if (pServer == nullptr || pServer->getConnectedCount() == 0) return;

      char buffer[MESSAGE_LENGTH];
      if(id=='E'||id=='I'){
          sprintf(buffer, "%c!%.2f", id, valf);
      } else {
          sprintf(buffer, "%c!%5d", id, vali);
      }
      pAllSensorsChar->setValue(buffer);
      pAllSensorsChar->notify();
  }

void checkToReconnect() //added
{
  // disconnected so advertise
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("Disconnected: start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connected so reset boolean control
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    Serial.println("Reconnected");
    oldDeviceConnected = deviceConnected;
  }
}


