#ifndef BLE_H
#define BLE_H

#ifdef __cplusplus
extern "C" {
#endif

void ble_init(const char *device_name);
void ble_update();

#ifdef __cplusplus
}
#endif

#endif