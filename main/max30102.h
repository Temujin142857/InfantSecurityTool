#ifndef MAX30102_H
#define GRANDPARENT_H

#include <stdint.h>
#include "esp_err.h"

typedef struct {
    uint32_t ir;
    uint32_t red;
} max_sample_t;

esp_err_t max30102_init(void);
int max30102_read_fifo(max_sample_t *samples, int max_samples);

#endif 