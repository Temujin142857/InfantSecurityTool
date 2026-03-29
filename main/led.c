#include "led.h"
#include "driver/gpio.h"


void led_init(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_ORANGE) | (1ULL << LED_RED),
    };

    gpio_config(&io_conf);
}

void gpio_toggle(gpio_num_t pin)
{
    int level = gpio_get_level(pin);
    gpio_set_level(pin, !level);
}