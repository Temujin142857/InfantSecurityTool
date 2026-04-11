#include "dht11.h"
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

void dht_init(){
    dht.begin();
}

void readHumidity(float *h){
  *h = dht.readHumidity();
  *h=*h+25;
}

void readTemperature(float *t){
  *t = dht.readTemperature();
  *t=*t;
}