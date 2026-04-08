#ifndef DHT11_H
#define DHT11_H

void dht_init();
void readHumidity(float *h);
void readTemperature(float *t);

#endif