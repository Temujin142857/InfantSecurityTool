#include <Arduino.h>
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "Mpu6050.h"
#include "dht11.h"
#include "max30102.h"

// =========================
// Messages
// =========================
enum SensorMessage {
  MSG_DHT = 1,
  MSG_MAX,
  MSG_MPU
};

// =========================
// Shared data
// =========================
static SemaphoreHandle_t dhtLock;
static SemaphoreHandle_t maxLock;
static SemaphoreHandle_t mpuLock;
static SemaphoreHandle_t i2cLock;
static QueueHandle_t sensorQueue;

// DHT11
static float g_humidity = 0.0f;
static float g_temperature = 0.0f;

// MAX30102
static int32_t g_heartRate = 0;
static int8_t  g_hrValid = 0;
static int32_t g_spo2 = 0;
static int8_t  g_spo2Valid = 0;

// MPU6050
static float g_motion = 0.0f;

// =========================
// I2C Scan
// =========================
void scanI2C() {
  Serial.println("Scanning I2C...");
  bool found = false;

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Found device at 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      found = true;
    }
  }

  if (!found) {
    Serial.println("No I2C device found");
  }

  Serial.println("Scan done.\n");
}

// =========================
// DHT11 Task
// =========================
void dhtTask(void *pvParameters) {
  for (;;) {
    float h = 0.0f;
    float t = 0.0f;

    readHumidity(&h);
    readTemperature(&t);

    if (!isnan(h) && !isnan(t)) {
      xSemaphoreTake(dhtLock, portMAX_DELAY);
      g_humidity = h;
      g_temperature = t;
      xSemaphoreGive(dhtLock);

      uint8_t msg = MSG_DHT;
      xQueueSend(sensorQueue, &msg, 0);
    } else {
      Serial.println("DHT11 read failed");
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// =========================
// MAX30102 Task
// =========================
void maxTask(void *pvParameters) {
  for (;;) {
    int32_t spo2 = 0;
    int8_t spo2Valid = 0;
    int32_t heartRate = 0;
    int8_t hrValid = 0;

    xSemaphoreTake(i2cLock, portMAX_DELAY);
    readHeartRateAndBloodOxygen(&spo2, &spo2Valid, &heartRate, &hrValid);
    xSemaphoreGive(i2cLock);

    xSemaphoreTake(maxLock, portMAX_DELAY);
    g_spo2 = spo2;
    g_spo2Valid = spo2Valid;
    g_heartRate = heartRate;
    g_hrValid = hrValid;
    xSemaphoreGive(maxLock);

    uint8_t msg = MSG_MAX;
    xQueueSend(sensorQueue, &msg, 0);

    vTaskDelay(pdMS_TO_TICKS(1500));
  }
}

// =========================
// MPU6050 Task
// =========================
void mpuTask(void *pvParameters) {
  for (;;) {
    float motion = 0.0f;

    xSemaphoreTake(i2cLock, portMAX_DELAY);
    readMotion(&motion);
    xSemaphoreGive(i2cLock);

    xSemaphoreTake(mpuLock, portMAX_DELAY);
    g_motion = motion;
    xSemaphoreGive(mpuLock);

    uint8_t msg = MSG_MPU;
    xQueueSend(sensorQueue, &msg, 0);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// =========================
// Serial Task
// =========================
void serialTask(void *pvParameters) {
  uint8_t msg;

  for (;;) {
    if (xQueueReceive(sensorQueue, &msg, portMAX_DELAY) == pdTRUE) {
      switch (msg) {
        case MSG_DHT: {
          float h, t;

          xSemaphoreTake(dhtLock, portMAX_DELAY);
          h = g_humidity;
          t = g_temperature;
          xSemaphoreGive(dhtLock);

          Serial.println("===== DHT11 =====");
          Serial.print("Humidity: ");
          Serial.println(h, 2);
          Serial.print("Temperature: ");
          Serial.println(t, 2);
          Serial.println("=================\n");
          break;
        }

        case MSG_MAX: {
          int32_t hr, spo2;
          int8_t hrValid, spo2Valid;

          xSemaphoreTake(maxLock, portMAX_DELAY);
          hr = g_heartRate;
          hrValid = g_hrValid;
          spo2 = g_spo2;
          spo2Valid = g_spo2Valid;
          xSemaphoreGive(maxLock);

          Serial.println("===== MAX30102 =====");
          Serial.print("Heart Rate: ");
          Serial.print(hr);
          Serial.print(" | valid = ");
          Serial.println(hrValid);

          Serial.print("SpO2: ");
          Serial.print(spo2);
          Serial.print(" | valid = ");
          Serial.println(spo2Valid);
          Serial.println("====================\n");
          break;
        }

        case MSG_MPU: {
          float motion;

          xSemaphoreTake(mpuLock, portMAX_DELAY);
          motion = g_motion;
          xSemaphoreGive(mpuLock);

          Serial.println("===== MPU6050 =====");
          Serial.print("Motion: ");
          Serial.println(motion, 4);
          Serial.println("===================\n");
          break;
        }
      }
    }
  }
}

// =========================
// Setup
// =========================
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("ESP32 start");

  // I2C for MAX30102 + MPU6050
  Wire.begin(21, 22);
  delay(500);

  scanI2C();

  // Init sensors
  dht_init();
  mpu_init(&Wire);
  max30102_init(&Wire);

  // Create mutexes
  dhtLock = xSemaphoreCreateMutex();
  maxLock = xSemaphoreCreateMutex();
  mpuLock = xSemaphoreCreateMutex();
  i2cLock = xSemaphoreCreateMutex();

  // Create queue
  sensorQueue = xQueueCreate(20, sizeof(uint8_t));

  if (!dhtLock || !maxLock || !mpuLock || !i2cLock || !sensorQueue) {
    Serial.println("Failed to create RTOS objects");
    while (1) {
      delay(1000);
    }
  }

  // Create tasks
  xTaskCreatePinnedToCore(dhtTask, "DHT Task", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(maxTask, "MAX Task", 8192, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(mpuTask, "MPU Task", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(serialTask, "Serial Task", 4096, NULL, 1, NULL, 0);
}

void loop() {
  // FreeRTOS tasks handle everything
}