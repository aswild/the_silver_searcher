#ifdef __FreeBSD__
#include <sys/endian.h>
#endif
#include <sys/types.h>

#ifdef __CYGWIN__
typedef _off64_t off64_t;
#endif

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_ERR_H
#include <err.h>
#endif
#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif
#ifdef HAVE_LZMA_H
#include <lzma.h>
#endif

#include "decompress.h"

#if HAVE_FOPENCOOKIE

#define min(a, b) ({                \
    __typeof (a) _a = (a);          \
    __typeof (b) _b = (b);          \
    _a < _b ? _a : _b; })

static cookie_read_function_t zfile_read;
static cookie_seek_function_t zfile_seek;
static cookie_close_function_t zfile_close;

static const cookie_io_functions_t zfile_io = {
    .read = zfile_read,
    .write = NULL,
    .seek = zfile_seek,
    .close = zfile_close,
};

#define KB (1024)
struct zfile {
    FILE *in;              // Source FILE stream
    uint64_t logic_offset, // Logical offset in output (forward seeks)
        decode_offset,     // Where we've decoded to
        actual_len;
    uint32_t outbuf_start;

    ag_compression_type ctype;

    union {
#ifdef HAVE_ZLIB_H
        z_stream gz;
#endif
#ifdef HAVE_LZMA_H
        lzma_stream lzma;
#endif
    } stream;

    uint8_t inbuf[32 * KB];
    uint8_t outbuf[256 * KB];
    bool eof;
    bool eof_next;
};

#ifdef HAVE_ZLIB_H
/***************** zlib functions ********************************/
static int zfile_zlib_cookie_init(struct zfile *cookie) {
    int rc;

    memset(&cookie->stream.gz, 0, sizeof(cookie->stream.gz));
    rc = inflateInit2(&cookie->stream.gz, 32 + 15);
    if (rc != Z_OK) {
        log_err("Unable to initialize zlib: %s", zError(rc));
        return EIO;
    }
    cookie->stream.gz.next_in = NULL;
    cookie->stream.gz.avail_in = 0;
    cookie->stream.gz.next_out = cookie->outbuf;
    cookie->stream.gz.avail_out = sizeof cookie->outbuf;
    return 0;
}

static inline size_t zfile_zlib_cavail_in(struct zfile *cookie) {
    return (size_t)cookie->stream.gz.avail_in;
}

static inline uint8_t *zfile_zlib_cnext_out(struct zfile *cookie) {
    return (uint8_t *)cookie->stream.gz.next_out;
}

static inline int zfile_zlib_is_stream_end(int ret) {
    return ret == Z_STREAM_END;
}

static inline void zfile_zlib_set_next_in(struct zfile *cookie, size_t nb) {
    cookie->stream.gz.avail_in = nb;
    cookie->stream.gz.next_in = cookie->inbuf;
}

static inline void zfile_zlib_set_next_out(struct zfile *cookie) {
    cookie->stream.gz.next_out = cookie->outbuf;
    cookie->stream.gz.avail_out = sizeof(cookie->outbuf);
}

static inline int zfile_zlib_inflate(struct zfile *cookie) {
    int ret = inflate(&cookie->stream.gz, Z_NO_FLUSH);
    if (ret == Z_STREAM_END) {
        cookie->eof_next = true;
        return 0;
    }
    return ret == Z_OK ? 0 : -1;
}

#else
/***************** zlib stubs ********************************/
#define zfile_zlib_cookie_init(c) EINVAL
#define zfile_zlib_cavail_in(c) 0
#define zfile_zlib_cnext_out(c) 0
#define zfile_zlib_is_stream_end(r) 1
#define zfile_zlib_set_next_in(c, n) (void)0
#define zfile_zlib_set_next_out(c) (void)0
#define zfile_zlib_inflate(c) -1
#endif

#ifdef HAVE_LZMA_H
/***************** lzma functions ********************************/
static int zfile_lzma_cookie_init(struct zfile *cookie) {
    lzma_ret lzrc;
    cookie->stream.lzma = (lzma_stream)LZMA_STREAM_INIT;
    lzrc = lzma_auto_decoder(&cookie->stream.lzma, -1, 0);
    if (lzrc != LZMA_OK) {
        log_err("Unable to initialize lzma_auto_decoder: %d", lzrc);
        return EIO;
    }
    cookie->stream.lzma.next_in = NULL;
    cookie->stream.lzma.avail_in = 0;
    cookie->stream.lzma.next_out = cookie->outbuf;
    cookie->stream.lzma.avail_out = sizeof(cookie->outbuf);
    return 0;
}

static inline size_t zfile_lzma_cavail_in(struct zfile *cookie) {
    return cookie->stream.lzma.avail_in;
}

static inline uint8_t *zfile_lzma_cnext_out(struct zfile *cookie) {
    return cookie->stream.lzma.next_out;
}

static inline int zfile_lzma_is_stream_end(int ret) {
    return (lzma_ret)ret == LZMA_STREAM_END;
}

static inline void zfile_lzma_set_next_in(struct zfile *cookie, size_t nb) {
    cookie->stream.lzma.avail_in = nb;
    cookie->stream.lzma.next_in = cookie->inbuf;
}

static inline void zfile_lzma_set_next_out(struct zfile *cookie) {
    cookie->stream.lzma.next_out = cookie->outbuf;
    cookie->stream.lzma.avail_out = sizeof(cookie->outbuf);
}

static inline int zfile_lzma_inflate(struct zfile *cookie) {
    lzma_ret ret = lzma_code(&cookie->stream.lzma, LZMA_RUN);
    if (ret == LZMA_STREAM_END) {
        cookie->eof_next = true;
        return 0;
    }
    return ret == LZMA_OK ? 0 : -1;
}

#else
/***************** lzma stubs ********************************/
#define zfile_lzma_cookie_init(c) EINVAL
#define zfile_lzma_cavail_in(c) 0
#define zfile_lzma_cnext_out(c) 0
#define zfile_lzma_is_stream_end(r) 1
#define zfile_lzma_set_next_in(c, n) (void)0
#define zfile_lzma_set_next_out(c) (void)0
#define zfile_lzma_inflate(c) -1
#endif

static inline size_t zfile_cavail_in(struct zfile *cookie) {
    switch (cookie->ctype) {
        case AG_GZIP:
            return zfile_zlib_cavail_in(cookie);
            break;
        case AG_XZ:
            return zfile_lzma_cavail_in(cookie);
            break;
        default:
            return 0;
    }
}

static inline uint8_t *zfile_cnext_out(struct zfile *cookie) {
    switch (cookie->ctype) {
        case AG_GZIP:
            return zfile_zlib_cnext_out(cookie);
            break;
        case AG_XZ:
            return zfile_lzma_cnext_out(cookie);
            break;
        default:
            return NULL;
    }
}

static inline int zfile_is_stream_end(struct zfile *cookie, int ret) {
    switch (cookie->ctype) {
        case AG_GZIP:
            return zfile_zlib_is_stream_end(ret);
        case AG_XZ:
            return zfile_lzma_is_stream_end(ret);
        default:
            return 1;
    }
}

static inline void zfile_set_next_in(struct zfile *cookie, size_t nb) {
    switch (cookie->ctype) {
        case AG_GZIP:
            zfile_zlib_set_next_in(cookie, nb);
            break;
        case AG_XZ:
            zfile_lzma_set_next_in(cookie, nb);
            break;
        default:
            break;
    }
}

static inline void zfile_set_next_out(struct zfile *cookie) {
    switch (cookie->ctype) {
        case AG_GZIP:
            zfile_zlib_set_next_out(cookie);
            break;
        case AG_XZ:
            zfile_lzma_set_next_out(cookie);
            break;
        default:
            break;
    }
}

static inline int zfile_inflate(struct zfile *cookie) {
    switch (cookie->ctype) {
        case AG_GZIP:
            return zfile_zlib_inflate(cookie);
            break;
        case AG_XZ:
            return zfile_lzma_inflate(cookie);
            break;
        default:
            return -1;
    }
}

static int
zfile_cookie_init(struct zfile *cookie) {
    int rc;

    assert(cookie->logic_offset == 0);
    assert(cookie->decode_offset == 0);

    cookie->actual_len = 0;

    switch (cookie->ctype) {
        case AG_GZIP:
            rc = zfile_zlib_cookie_init(cookie);
            break;
        case AG_XZ:
            rc = zfile_lzma_cookie_init(cookie);
            break;
        default:
            log_err("Unsupported compression type: %d", cookie->ctype);
            return EINVAL;
    }
    if (rc)
        return rc;

    cookie->outbuf_start = 0;
    cookie->eof = false;
    return 0;
}

static void
zfile_cookie_cleanup(struct zfile *cookie) {
    switch (cookie->ctype) {
#ifdef HAVE_ZLIB_H
        case AG_GZIP:
            inflateEnd(&cookie->stream.gz);
            break;
#endif
#ifdef HAVE_LZMA_H
        case AG_XZ:
            lzma_end(&cookie->stream.lzma);
            break;
#endif
        default:
            /* Compiler false positive - unreachable. */
            break;
    }
}

/*
 * Open compressed file 'path' as a (forward-)seekable (and rewindable),
 * read-only stream.
 */
FILE *
decompress_open(int fd, const char *mode, ag_compression_type ctype) {
    struct zfile *cookie;
    FILE *res, *in;
    int error;

    cookie = NULL;
    in = res = NULL;
    if (strstr(mode, "w") || strstr(mode, "a")) {
        errno = EINVAL;
        goto out;
    }

    in = fdopen(fd, mode);
    if (in == NULL)
        goto out;

    /*
     * No validation of compression type is done -- file is assumed to
     * match input.  In Ag, the compression type is already detected, so
     * that's ok.
     */
    cookie = malloc(sizeof *cookie);
    if (cookie == NULL) {
        errno = ENOMEM;
        goto out;
    }

    cookie->in = in;
    cookie->logic_offset = 0;
    cookie->decode_offset = 0;
    cookie->ctype = ctype;

    error = zfile_cookie_init(cookie);
    if (error != 0) {
        errno = error;
        goto out;
    }

    res = fopencookie(cookie, mode, zfile_io);

out:
    if (res == NULL) {
        if (in != NULL)
            fclose(in);
        if (cookie != NULL)
            free(cookie);
    }
    return res;
}

/*
 * Return number of bytes into buf, 0 on EOF, -1 on error.  Update stream
 * offset.
 */
static ssize_t
zfile_read(void *cookie_, char *buf, size_t size) {
    struct zfile *cookie = cookie_;
    size_t nb, ignorebytes;
    ssize_t total = 0;

    assert(size <= SSIZE_MAX);

    if (size == 0)
        return 0;

    if (cookie->eof)
        return 0;

    ignorebytes = cookie->logic_offset - cookie->decode_offset;
    assert(ignorebytes == 0);

    do {
        size_t inflated;

        /* Drain output buffer first */
        while (zfile_cnext_out(cookie) >
               &cookie->outbuf[cookie->outbuf_start]) {
            size_t left = zfile_cnext_out(cookie) -
                          &cookie->outbuf[cookie->outbuf_start];
            size_t ignoreskip = min(ignorebytes, left);
            size_t toread;

            if (ignoreskip > 0) {
                ignorebytes -= ignoreskip;
                left -= ignoreskip;
                cookie->outbuf_start += ignoreskip;
                cookie->decode_offset += ignoreskip;
            }

            // Ran out of output before we seek()ed up.
            if (ignorebytes > 0)
                break;

            toread = min(left, size);
            memcpy(buf, &cookie->outbuf[cookie->outbuf_start],
                   toread);

            buf += toread;
            size -= toread;
            left -= toread;
            cookie->outbuf_start += toread;
            cookie->decode_offset += toread;
            cookie->logic_offset += toread;
            total += toread;

            if (size == 0)
                break;
        }

        if (size == 0)
            break;

        if (cookie->eof_next) {
            cookie->eof = true;
            break;
        }

        /* Read more input if empty */
        if (zfile_cavail_in(cookie) == 0) {
            nb = fread(cookie->inbuf, 1, sizeof cookie->inbuf,
                       cookie->in);
            if (ferror(cookie->in)) {
                warn("error read core");
                exit(1);
            }
            if (nb == 0 && feof(cookie->in)) {
                warn("truncated file");
                exit(1);
            }
            zfile_set_next_in(cookie, nb);
        }

        /* Reset stream state to beginning of output buffer */
        zfile_set_next_out(cookie);
        cookie->outbuf_start = 0;

        if (zfile_inflate(cookie)) {
            log_err("Found mem/data error while decompressing stream");
            return -1;
        }
        inflated = zfile_cnext_out(cookie) - &cookie->outbuf[0];
        cookie->actual_len += inflated;
    } while (!ferror(cookie->in) && size > 0);

    assert(total <= SSIZE_MAX);
    return total;
}

static int
zfile_seek(void *cookie_, off64_t *offset_, int whence) {
    struct zfile *cookie = cookie_;
    off64_t new_offset = 0, offset = *offset_;

    if (whence == SEEK_SET) {
        new_offset = offset;
    } else if (whence == SEEK_CUR) {
        new_offset = (off64_t)cookie->logic_offset + offset;
    } else {
        /* SEEK_END not ok */
        return -1;
    }

    if (new_offset < 0)
        return -1;

    /* Backward seeks to anywhere but 0 are not ok */
    if (new_offset < (off64_t)cookie->logic_offset && new_offset != 0) {
        return -1;
    }

    if (new_offset == 0) {
        /* rewind(3) */
        cookie->decode_offset = 0;
        cookie->logic_offset = 0;
        zfile_cookie_cleanup(cookie);
        zfile_cookie_init(cookie);
    } else if ((uint64_t)new_offset > cookie->logic_offset) {
        /* Emulate forward seek by skipping ... */
        char *buf;
        const size_t bsz = 32 * 1024;

        buf = malloc(bsz);
        while ((uint64_t)new_offset > cookie->logic_offset) {
            size_t diff = min(bsz,
                              (uint64_t)new_offset - cookie->logic_offset);
            ssize_t err = zfile_read(cookie_, buf, diff);
            if (err < 0) {
                free(buf);
                return -1;
            }

            /* Seek past EOF gets positioned at EOF */
            if (err == 0) {
                assert(cookie->eof);
                new_offset = cookie->logic_offset;
                break;
            }
        }
        free(buf);
    }

    assert(cookie->logic_offset == (uint64_t)new_offset);

    *offset_ = new_offset;
    return 0;
}

static int
zfile_close(void *cookie_) {
    struct zfile *cookie = cookie_;

    zfile_cookie_cleanup(cookie);
    fclose(cookie->in);
    free(cookie);

    return 0;
}

#endif /* HAVE_FOPENCOOKIE */
