#include <Wire.h>
#include <MPU6050.h>
#include <DHT.h>

MPU6050 mpu;

// LEDs
#define LED_MPU 2
#define LED_DHT 5

// DHT11
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

float last = 0;

// mouvement MPU
float motion(int16_t ax, int16_t ay, int16_t az) {
  float current = abs(ax) + abs(ay) + abs(az);
  float delta = abs(current - last);
  last = current;
  return delta;
}

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  mpu.initialize();

  dht.begin();

  pinMode(LED_MPU, OUTPUT);
  pinMode(LED_DHT, OUTPUT);

  Serial.println("Systeme initialise !");
}

void loop() {
  // ===== MPU6050 =====
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  float m = motion(ax, ay, az);

  // ===== DHT11 =====
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // ===== Affichage =====
  Serial.print("Motion: ");
  Serial.print(m);

  Serial.print(" | Temp: ");
  Serial.print(temp);

  Serial.print(" | Hum: ");
  Serial.println(hum);

  // ===== LED MPU =====
  if (m > 2000) digitalWrite(LED_MPU, HIGH);
  else digitalWrite(LED_MPU, LOW);

  // ===== LED DHT =====
  if (temp > 30) digitalWrite(LED_DHT, HIGH);
  else digitalWrite(LED_DHT, LOW);

  delay(500);
}
