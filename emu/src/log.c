#include <util/log.h>

#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static log_level_t g_level = LOG_INFO;

typedef struct level_style
{
    const char *name;
    const char *color;
} level_style_t;

static const level_style_t g_styles[] = {
    {"TRACE", "\033[90m"}, /* gray */
    {"DEBUG", "\033[36m"}, /* cyan */
    {"INFO ", "\033[32m"}, /* green */
    {"WARN ", "\033[33m"}, /* yellow */
    {"ERROR", "\033[31m"}, /* red */
};

static bool use_color(void)
{
    static int tty = -1;
    if (tty < 0)
        tty = isatty(fileno(stderr));
    return tty != 0;
}

static void timestamp(char *buf, size_t len)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm;
    localtime_r(&tv.tv_sec, &tm);
    snprintf(buf, len, "%02d:%02d:%02d.%03d", tm.tm_hour, tm.tm_min, tm.tm_sec, (int)(tv.tv_usec / 1000));
}

void log_set_level(log_level_t level)
{
    g_level = level;
}

log_level_t log_get_level(void)
{
    return g_level;
}

bool log_enabled(log_level_t level)
{
    return level >= g_level;
}

void log_write(log_level_t level, const char *fmt, ...)
{
    if (!log_enabled(level))
        return;

    char ts[16];
    timestamp(ts, sizeof ts);

    const level_style_t *s = &g_styles[level];
    if (use_color())
        fprintf(stderr, "\033[90m%s\033[0m %s%s\033[0m ", ts, s->color, s->name);
    else
        fprintf(stderr, "%s %s ", ts, s->name);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);
}
