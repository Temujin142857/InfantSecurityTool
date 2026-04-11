#include <WiFi.h>
#include <WebServer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "wifi.h"

// ── PLACEHOLDER CREDENTIALS — replace before flashing ─────────────────────────
#define WIFI_SSID     "YOUR_SSID_HERE"       // <-- replace
#define WIFI_PASSWORD "YOUR_PASSWORD_HERE"   // <-- replace
// ─────────────────────────────────────────────────────────────────────────────

#define SLOT_SIZE 9   // MESSAGE_LENGTH (8) + null terminator

typedef struct {
	char id;
	char buf[SLOT_SIZE];
	bool set;
} SensorSlot;

static SensorSlot slots[] = {
	{'E', "E!    0", false},
	{'I', "I!    0", false},
	{'B', "B!    0", false},
	{'C', "C!    0", false},
	{'O', "O!    0", false},
	{'S', "S!    0", false},
	{'H', "H!    0", false},
	{'M', "M!    0", false},
};
static const uint8_t SENSOR_COUNT = 8;

static SemaphoreHandle_t slotsLock;
static WebServer         server(80);

// ── HTTP handlers ─────────────────────────────────────────────────────────────

static void handlePing() {
	server.send(200, "text/plain", "OK");
}

static void handleSensors() {
	// Build payload by joining filled slots with ';'
	// Max size: SENSOR_COUNT * (SLOT_SIZE + 1) + 1
	char payload[SENSOR_COUNT * (SLOT_SIZE + 1) + 1];
	payload[0] = '\0';

	xSemaphoreTake(slotsLock, portMAX_DELAY);
	bool first = true;
	for (int i = 0; i < SENSOR_COUNT; i++) {
		if (!slots[i].set) continue;
		if (!first) strncat(payload, ";", sizeof(payload) - strlen(payload) - 1);
		strncat(payload, slots[i].buf, sizeof(payload) - strlen(payload) - 1);
		first = false;
	}
	xSemaphoreGive(slotsLock);

	server.send(200, "text/plain", payload);
}

// ── Internal server task ──────────────────────────────────────────────────────

static void wifiServerTask(void *pvParameter) {
	for (;;) {
		server.handleClient();
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

// ── Public API ────────────────────────────────────────────────────────────────

void wifi_init() {
	slotsLock = xSemaphoreCreateMutex();

	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

	Serial.print("[WiFi] Connecting");
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println();
	Serial.print("[WiFi] Connected, IP: ");
	Serial.println(WiFi.localIP());

	server.on("/ping",    handlePing);
	server.on("/sensors", handleSensors);
	server.begin();
	Serial.println("[WiFi] HTTP server started on port 80");

	xTaskCreatePinnedToCore(wifiServerTask, "wifiServer", 4096, NULL, 1, NULL, 0);
}

void wifi_send_sensor(char id, float valf, int vali) {
	char buffer[SLOT_SIZE];
	if (id == 'E' || id == 'I')
		sprintf(buffer, "%c!%.2f", id, valf);
	else
		sprintf(buffer, "%c!%5d", id, vali);

	xSemaphoreTake(slotsLock, portMAX_DELAY);
	for (int i = 0; i < SENSOR_COUNT; i++) {
		if (slots[i].id == id) {
			memcpy(slots[i].buf, buffer, SLOT_SIZE);
			slots[i].set = true;
			break;
		}
	}
	xSemaphoreGive(slotsLock);
}
