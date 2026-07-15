#pragma once

#include <stdbool.h>

typedef enum log_level
{
    LOG_TRACE = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
} log_level_t;

void log_set_level(log_level_t level);
log_level_t log_get_level(void);
bool log_enabled(log_level_t level);

void log_write(log_level_t level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#define log_trace(...) log_write(LOG_TRACE, __VA_ARGS__)
#define log_debug(...) log_write(LOG_DEBUG, __VA_ARGS__)
#define log_info(...) log_write(LOG_INFO, __VA_ARGS__)
#define log_warn(...) log_write(LOG_WARN, __VA_ARGS__)
#define log_error(...) log_write(LOG_ERROR, __VA_ARGS__)
