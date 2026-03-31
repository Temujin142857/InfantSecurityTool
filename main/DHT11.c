#include "dht11.h"
#include "esp_rom_sys.h"

static int wait_for_level(gpio_num_t pin, int level, int timeout_us)
{
    int count = 0;
    while (gpio_get_level(pin) == level) {
        if (++count > timeout_us) return -1;
        esp_rom_delay_us(1);
    }
    return count;
}

esp_err_t dht11_read(gpio_num_t pin, dht11_data_t *out)
{
    uint8_t data[5] = {0};

    // --- Start signal ---
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    esp_rom_delay_us(18000); // 18 ms

    gpio_set_level(pin, 1);
    esp_rom_delay_us(30);

    gpio_set_direction(pin, GPIO_MODE_INPUT);

    // --- Sensor response ---
    if (wait_for_level(pin, 0, 100) < 0) return ESP_FAIL;
    if (wait_for_level(pin, 1, 100) < 0) return ESP_FAIL;
    if (wait_for_level(pin, 0, 100) < 0) return ESP_FAIL;

    // --- Read 40 bits ---
    for (int i = 0; i < 40; i++) {
        if (wait_for_level(pin, 1, 100) < 0) return ESP_FAIL;

        int high_time = wait_for_level(pin, 0, 100);
        if (high_time < 0) return ESP_FAIL;

        // Bit value
        data[i / 8] <<= 1;
        if (high_time > 40) { // threshold
            data[i / 8] |= 1;
        }
    }

    // --- Check checksum ---
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return ESP_FAIL;
    }

    out->humidity = data[0];
    out->temperature = data[2];

    return ESP_OK;
}