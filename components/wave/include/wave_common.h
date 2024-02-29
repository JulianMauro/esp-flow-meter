#pragma once

#include <stdint.h>
#include <stdio.h>

#define WAVE_HEADER_SIZE             (44)

typedef struct{
    uint16_t n_channels;
    uint16_t sample_rate;
    uint32_t samples_per_channel;
    uint16_t bytes_per_sample;
}wave_header_t;

void wave_print_header(const wave_header_t *header, uint8_t *buffer);