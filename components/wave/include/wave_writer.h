#pragma once

#include "wave_common.h"

typedef struct wave_writer_t* wave_writer_handle_t;

wave_writer_handle_t wave_writer_open(const char *filename);
int wave_write_header(wave_writer_handle_t writer, const wave_header_t *header);
size_t wave_write_raw_data(wave_writer_handle_t writer, const uint8_t* data, size_t size);
int wave_refresh_header_size_fields(wave_writer_handle_t writer);
int wave_writer_commit(wave_writer_handle_t writer);
int wave_writer_close(wave_writer_handle_t writer);