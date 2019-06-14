#ifndef LOG_H
#define LOG_H

#include "config.h"

#include <stdarg.h>
#include <pthread.h>

#include "compiler.h"

extern pthread_mutex_t print_mtx;

enum log_level {
    LOG_LEVEL_DEBUG = 10,
    LOG_LEVEL_MSG = 20,
    LOG_LEVEL_WARN = 30,
    LOG_LEVEL_ERR = 40,
    LOG_LEVEL_NONE = 100
};

void set_log_level(enum log_level threshold);

void _log_debug(const char *fmt, ...) ATTR_FORMAT_PRINTF(1, 2);
#define log_debug(fmt, args...) _log_debug("%s:%d: " fmt, __func__, __LINE__, ##args)

void log_msg(const char *fmt, ...) ATTR_FORMAT_PRINTF(1, 2);
void log_warn(const char *fmt, ...) ATTR_FORMAT_PRINTF(1, 2);
void log_err(const char *fmt, ...) ATTR_FORMAT_PRINTF(1, 2);

void vplog(const unsigned int level, const char *fmt, va_list args);
void plog(const unsigned int level, const char *fmt, ...);

#endif
