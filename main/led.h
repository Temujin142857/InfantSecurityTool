#ifndef LED_H
#define LED_H
#include "driver/gpio.h"

#define LED_ORANGE GPIO_NUM_5
#define LED_RED    GPIO_NUM_3

void gpio_toggle(gpio_num_t pin);

void led_init(void);

#endif