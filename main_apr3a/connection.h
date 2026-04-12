#ifndef WIFI_MODULE_DECL_H
#define WIFI_MODULE_DECL_H

// On Windows, wifi.h and WiFi.h are the same filename (case-insensitive
// filesystem), so #include <WiFi.h> in wifi.cpp finds this file first.
// #include_next tells GCC to skip this file and find the real Arduino WiFi.h
// that is next in the compiler's search path.
#include <WiFi.h>

void wifi_init();
void wifi_send_sensor(char id, float valf, int vali);

#endif
