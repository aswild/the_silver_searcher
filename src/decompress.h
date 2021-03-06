#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include <stdio.h>

#include "config.h"
#include "log.h"
#include "options.h"

typedef enum {
    AG_NO_COMPRESSION,
    AG_GZIP,
    AG_XZ,
    AG_LIBARCHIVE,
} ag_compression_type;

ag_compression_type is_zipped(const void *buf, const size_t buf_len);

void *decompress(const ag_compression_type zip_type, const void *buf, const size_t buf_len, const char *dir_full_path, size_t *new_buf_len);

#endif
