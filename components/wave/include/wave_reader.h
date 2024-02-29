#pragma once

#include "wave_common.h"

typedef struct wave_reader_t* wave_reader_handle_t;

wave_reader_handle_t wave_reader_open(const char *filename);
int wave_read_header(wave_reader_handle_t reader, wave_header_t *header);
size_t wave_read_raw_data(wave_reader_handle_t reader, uint8_t* data, size_t pos, size_t size);
int wave_reader_close(wave_reader_handle_t reader);