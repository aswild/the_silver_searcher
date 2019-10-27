#include "config.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#ifdef USE_ZLIB
#define ZLIB_CONST 1
#include <zlib.h>
#endif

#ifdef USE_LZMA
#include <lzma.h>
/*  http://tukaani.org/xz/xz-file-format.txt */
static const uint8_t XZ_HEADER_MAGIC[6] = { 0xFD, '7', 'z', 'X', 'Z', 0x00 };
static const uint8_t LZMA_HEADER_SOMETIMES[3] = { 0x5D, 0x00, 0x00 };
#endif

#ifdef USE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

#include "decompress.h"
#include "util.h"

static size_t pagesize = 0;
ALWAYS_INLINE static inline size_t round_up_page(size_t sz) {
    if (pagesize == 0)
        pagesize = getpagesize();
    return (sz + pagesize - 1) & ~(pagesize - 1);
}

/* Code in decompress_zlib from
 *
 * https://raw.github.com/madler/zlib/master/examples/zpipe.c
 *
 * zpipe.c: example of proper use of zlib's inflate() and deflate()
 *    Not copyrighted -- provided to the public domain
 *    Version 1.4  11 December 2005  Mark Adler
 */
#ifdef USE_ZLIB
static void *decompress_zlib(const void *buf, const size_t buf_len,
                             const char *dir_full_path, size_t *new_buf_len) {
    int ret = 0;
    unsigned char *result = NULL;
    size_t result_size = round_up_page(buf_len);
    z_stream stream;

    log_debug("Decompressing zlib file %s", dir_full_path);

    /* allocate inflate state */
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;

    /* Add 32 to allow zlib and gzip format detection */
    if (inflateInit2(&stream, 32 + 15) != Z_OK) {
        log_err("Unable to initialize zlib: %s", stream.msg);
        goto error_out;
    }

    stream.avail_in = buf_len;
    stream.next_in = buf;

    do {
        do {
            unsigned char *tmp_result = result;
            /* Double the buffer size and realloc */
            result_size *= 2;
            result = (unsigned char *)realloc(result, result_size * sizeof(unsigned char));
            if (result == NULL) {
                free(tmp_result);
                log_err("Unable to allocate %zu bytes to decompress file %s", result_size * sizeof(unsigned char), dir_full_path);
                inflateEnd(&stream);
                goto error_out;
            }

            stream.avail_out = result_size / 2;
            stream.next_out = &result[stream.total_out];
            ret = inflate(&stream, Z_SYNC_FLUSH);
            log_debug("inflate ret = %d", ret);
            switch (ret) {
                case Z_STREAM_ERROR: {
                    log_err("Found stream error while decompressing zlib stream: %s", stream.msg);
                    inflateEnd(&stream);
                    goto error_out;
                }
                case Z_NEED_DICT:
                case Z_DATA_ERROR:
                case Z_MEM_ERROR: {
                    log_err("Found mem/data error while decompressing zlib stream: %s", stream.msg);
                    inflateEnd(&stream);
                    goto error_out;
                }
            }
        } while (stream.avail_out == 0);
    } while (ret == Z_OK);

    *new_buf_len = stream.total_out;
    inflateEnd(&stream);

    if (ret == Z_STREAM_END) {
        return result;
    }

error_out:
    *new_buf_len = 0;
    return NULL;
}
#endif // USE_ZLIB

#ifdef USE_LZMA
static void *decompress_lzma(const void *buf, const size_t buf_len,
                             const char *dir_full_path, size_t *new_buf_len) {
    lzma_stream stream = LZMA_STREAM_INIT;
    lzma_ret lzrt;
    unsigned char *result = NULL;
    size_t result_size = round_up_page(buf_len);

    stream.avail_in = buf_len;
    stream.next_in = buf;

    lzrt = lzma_auto_decoder(&stream, -1, 0);

    if (lzrt != LZMA_OK) {
        log_err("Unable to initialize lzma_auto_decoder: %d", lzrt);
        goto error_out;
    }

    do {
        do {
            unsigned char *tmp_result = result;
            /* Double the buffer size and realloc */
            result_size *= 2;
            result = (unsigned char *)realloc(result, result_size * sizeof(unsigned char));
            if (result == NULL) {
                free(tmp_result);
                log_err("Unable to allocate %zu bytes to decompress file %s", result_size * sizeof(unsigned char), dir_full_path);
                goto error_out;
            }

            stream.avail_out = result_size / 2;
            stream.next_out = &result[stream.total_out];
            lzrt = lzma_code(&stream, LZMA_RUN);
            log_debug("lzma_code ret = %d", lzrt);
            switch (lzrt) {
                case LZMA_OK:
                case LZMA_STREAM_END:
                    break;
                default:
                    log_err("Found mem/data error while decompressing xz/lzma stream: %d", lzrt);
                    goto error_out;
            }
        } while (stream.avail_out == 0);
    } while (lzrt == LZMA_OK);

    *new_buf_len = stream.total_out;

    if (lzrt == LZMA_STREAM_END) {
        lzma_end(&stream);
        return result;
    }


error_out:
    lzma_end(&stream);
    *new_buf_len = 0;
    if (result) {
        free(result);
    }
    return NULL;
}
#endif // USE_LZMA

#ifdef USE_LIBARCHIVE
static void *decompress_libarchive(const void *buf, const size_t buf_len,
                                   const char *dir_full_path, size_t *new_buf_len) {
    struct archive *a = NULL;
    struct archive_entry *ae = NULL;
    size_t total_read = 0;
    size_t result_size = round_up_page(buf_len);

    unsigned char *result = malloc(result_size);
    if (result == NULL) {
        log_err("Unable to malloc %zu bytes to decompress file %s", result_size, dir_full_path);
        goto error_out;
    }

    a = archive_read_new();
    if (a == NULL) {
        log_err("Unable to initialize libarchive");
        goto error_out;
    }

    archive_read_support_format_raw(a);
    archive_read_support_filter_all(a);
    if (archive_read_open_memory(a, buf, buf_len) != ARCHIVE_OK) {
        log_err("Unable to open libarchive stream: %s", archive_error_string(a));
        goto error_out;
    }
    if (archive_read_next_header(a, &ae) != ARCHIVE_OK) {
        log_err("Unable to read libarchive header entry: %s", archive_error_string(a));
        goto error_out;
    }

    while (true) {
        const void *block;
        size_t block_size;
        off_t block_off;
        int r = archive_read_data_block(a, &block, &block_size, &block_off);
        if (r == ARCHIVE_EOF) {
            break;
        } else if (r != ARCHIVE_OK) {
            log_err("Failed decompressing file %s with libarchive: %s", dir_full_path, archive_error_string(a));
            goto error_out;
        }

        total_read = block_off + block_size;
        while (result_size < total_read) {
            unsigned char *result_save = result;
            result_size *= 2;
            result = realloc(result, result_size);
            if (result == NULL) {
                log_err("Unable to realloc %zu bytes to decompress file %s", result_size, dir_full_path);
                free(result_save);
                goto error_out;
            }
        }

        memcpy(result + block_off, block, block_size);
    }

    archive_read_free(a);
    *new_buf_len = total_read;
    return result;

error_out:
    *new_buf_len = 0;
    free(result);
    if (a != NULL) {
        archive_read_free(a);
    }
    return NULL;
}
#endif // USE_LIBARCHIVE


/* This function is very hot. It's called on every file when zip is enabled. */
void *decompress(const ag_compression_type zip_type, const void *buf, const size_t buf_len,
                 const char *dir_full_path, size_t *new_buf_len) {
    /* suppress unused-parameter warnings if zlib and lzma are disabled */
    (void)buf;
    (void)buf_len;

    switch (zip_type) {
#ifdef USE_ZLIB
        case AG_GZIP:
            return decompress_zlib(buf, buf_len, dir_full_path, new_buf_len);
#endif
#ifdef USE_LZMA
        case AG_XZ:
            return decompress_lzma(buf, buf_len, dir_full_path, new_buf_len);
#endif
#ifdef USE_LIBARCHIVE
        case AG_LIBARCHIVE:
            return decompress_libarchive(buf, buf_len, dir_full_path, new_buf_len);
#endif
        case AG_NO_COMPRESSION:
            log_err("File %s is not compressed", dir_full_path);
            break;
        default:
            log_err("Unsupported compression type: %d", zip_type);
    }

    *new_buf_len = 0;
    return NULL;
}


/* This function is very hot. It's called on every file. */
ag_compression_type is_zipped(const void *buf, const size_t buf_len) {
    /* Zip magic numbers
     * gzip file:       { 0x1F, 0x8B }
     * http://www.gzip.org/zlib/rfc-gzip.html#file-format
     *
     * zip file:        { 0x50, 0x4B, 0x03, 0x04 }
     * http://www.pkware.com/documents/casestudies/APPNOTE.TXT (Section 4.3)
     */

    const unsigned char *const buf_c = buf;
    (void)buf_c; /* suppress unused variable warning if all compression modes are disabled */

    if (buf_len == 0) {
        return AG_NO_COMPRESSION;
    }

    /* Check for gzip & compress */
#ifdef USE_ZLIB
    if ((buf_len >= 2) && (buf_c[0] == 0x1F) && (buf_c[1] == 0x8B)) {
        log_debug("Found gzip-based stream");
        return AG_GZIP;
    }
#endif

#ifdef USE_LZMA
    if (buf_len >= 6) {
        if (memcmp(XZ_HEADER_MAGIC, buf_c, 6) == 0) {
            log_debug("Found xz based stream");
            return AG_XZ;
        }
    }

    /* LZMA doesn't really have a header: http://www.mail-archive.com/xz-devel@tukaani.org/msg00003.html */
    if (buf_len >= 3) {
        if (memcmp(LZMA_HEADER_SOMETIMES, buf_c, 3) == 0) {
            log_debug("Found lzma-based stream");
            return AG_XZ;
        }
    }
#endif

    /* check for libarchive support */
#ifdef USE_LIBARCHIVE
    struct archive *a = archive_read_new();
    if (a == NULL) {
        log_err("Failed to initialize libarchive");
        return AG_NO_COMPRESSION;
    }
    /* check for all compression filters, but only in the raw format
     * so that libarchive doesn't try to check for tar files and what not */
    archive_read_support_format_raw(a);
    archive_read_support_filter_all(a);

    /* open the libarchive stream to check whether it's compressed, then close/free it.
     * TODO: keep the libarchive stream open and pass it back to the search code */
    bool la_supported = false;
    int r = archive_read_open_memory(a, buf_c, buf_len);
    if (r == ARCHIVE_OK) {
        la_supported = archive_filter_code(a, 0) != ARCHIVE_FILTER_NONE;
    } else {
        log_err("Failed to check compression type with libarchive");
    }
    archive_read_free(a); /* automatically calls archive_read_close */
    return la_supported ? AG_LIBARCHIVE : AG_NO_COMPRESSION;
#endif

    return AG_NO_COMPRESSION;
}
