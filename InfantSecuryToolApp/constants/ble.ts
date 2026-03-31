// ─── ESP32 BLE UUIDs ────────────────────────────────────────────────────────
// These must match the Service / Characteristic UUIDs in your ESP32 firmware.
// Replace the placeholder UUIDs below with the actual ones from your firmware.

export const ESP32_SERVICE_UUID = '12345678-1234-1234-1234-1234567890ab';

export const CHARACTERISTIC_UUIDS = {
  temperature: '12345678-1234-1234-1234-1234567890ac',
  humidity:    '12345678-1234-1234-1234-1234567890ad',
  motion:      '12345678-1234-1234-1234-1234567890ae',
  pulse:       '12345678-1234-1234-1234-1234567890af',
  sound:       '12345678-1234-1234-1234-1234567890b0',
};

// ─── Sensor Data Types ───────────────────────────────────────────────────────

export type SensorKey = keyof typeof CHARACTERISTIC_UUIDS;

export interface SensorData {
  temperature: number | null; // °C
  humidity:    number | null; // %
  motion:      number | null; // 1 = motion detected, 0 = no motion; seconds since last motion
  pulse:       number | null; // bpm
  sound:       number | null; // dB level
}

export const DEFAULT_SENSOR_DATA: SensorData = {
  temperature: null,
  humidity:    null,
  motion:      null,
  pulse:       null,
  sound:       null,
};

// ─── Notification Thresholds ─────────────────────────────────────────────────

export const THRESHOLDS = {
  temperature: { min: 15, max: 38 },   // °C
  humidity:    { min: 20, max: 80 },   // %
  motion:      { noMotionSeconds: 30 },// seconds without motion = alert
  pulse:       { min: 60, max: 160 },  // bpm
  sound:       { max: 80 },            // dB
};

// ─── Sensor Display Config ───────────────────────────────────────────────────

export const SENSOR_CONFIG: Record<SensorKey, { label: string; unit: string; icon: string }> = {
  temperature: { label: 'Temperature', unit: '°C',  icon: '🌡️' },
  humidity:    { label: 'Humidity',    unit: '%',   icon: '💧' },
  motion:      { label: 'Motion',      unit: 's',   icon: '🏃' },
  pulse:       { label: 'Heart Rate',  unit: 'bpm', icon: '❤️' },
  sound:       { label: 'Sound',       unit: 'dB',  icon: '🔊' },
};
