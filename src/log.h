#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

#include "config.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

// GCC attribute to produce -Wformat warnings for printf-like functions
#ifdef __GNUC__
#define PRINTF_ATTR __attribute__((format(printf, 1, 2)))
#else
#define PRINTF_ATTR
#endif

pthread_mutex_t print_mtx;

enum log_level {
    LOG_LEVEL_DEBUG = 10,
    LOG_LEVEL_MSG = 20,
    LOG_LEVEL_WARN = 30,
    LOG_LEVEL_ERR = 40,
    LOG_LEVEL_NONE = 100
};

void set_log_level(enum log_level threshold);

void _log_debug(const char *fmt, ...) PRINTF_ATTR;
#define log_debug(fmt, args...) _log_debug("%s:%d: " fmt, __func__, __LINE__, ##args)

void log_msg(const char *fmt, ...) PRINTF_ATTR;
void log_warn(const char *fmt, ...) PRINTF_ATTR;
void log_err(const char *fmt, ...) PRINTF_ATTR;

void vplog(const unsigned int level, const char *fmt, va_list args);
void plog(const unsigned int level, const char *fmt, ...);

#endif
