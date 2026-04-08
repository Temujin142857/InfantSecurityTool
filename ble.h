#ifndef BLE_H
#define BLE_H

#define MESSAGE_LENGTH 8


void ble_init(const char *device_name);
void ble_set_all_sensors(char id, float valf, int vali);

#endif