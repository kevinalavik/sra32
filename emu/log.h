#ifndef SRA32_LOG_H
#define SRA32_LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef enum
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
} log_level_t;

static log_level_t log_min_level = LOG_INFO;

static inline void log_set_level(log_level_t level)
{
    log_min_level = level;
}

static inline int log_color_enabled(void)
{
    static int enabled = -1;
    if (enabled < 0)
        enabled = isatty(STDERR_FILENO) && !getenv("NO_COLOR");
    return enabled;
}

static inline void log_msg(log_level_t level, const char *tag, const char *fmt, ...)
{
    static const char *names[] = {"debug", "info", "warn", "error"};
    static const char *colors[] = {"\033[90m", "\033[36m", "\033[33m", "\033[31m"};
    va_list ap;

    if (level < log_min_level)
        return;

    if (log_color_enabled())
        fprintf(stderr, "%s%-5s\033[0m ", colors[level], names[level]);
    else
        fprintf(stderr, "%-5s ", names[level]);

    if (tag)
        fprintf(stderr, "%s: ", tag);

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

#define log_debug(tag, ...) log_msg(LOG_DEBUG, (tag), __VA_ARGS__)
#define log_info(tag, ...) log_msg(LOG_INFO, (tag), __VA_ARGS__)
#define log_warn(tag, ...) log_msg(LOG_WARN, (tag), __VA_ARGS__)
#define log_error(tag, ...) log_msg(LOG_ERROR, (tag), __VA_ARGS__)

#define log_fatal(tag, ...)                     \
    do                                          \
    {                                           \
        log_msg(LOG_ERROR, (tag), __VA_ARGS__); \
        exit(EXIT_FAILURE);                     \
    } while (0)

#endif // SRA32_LOG_H
