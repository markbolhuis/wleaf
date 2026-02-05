#include <stdio.h>

#include "log_priv.h"

static void
wlf_log_stderr(const struct wlf_log_info *info, const char *fmt, va_list ap);

static enum wlf_log_level g_log_level = WLF_LOG_DEBUG;
static wlf_log_func_t g_log_func = wlf_log_stderr;

static void
wlf_log_stderr(
    const struct wlf_log_info *info,
    const char *fmt,
    va_list ap)
{
    if (info->level > g_log_level)
        return;

    switch (info->level) {
        case WLF_LOG_ERROR:
            fprintf(stderr, "ERROR ");
            break;
        case WLF_LOG_WARNING:
            fprintf(stderr, " WARN ");
            break;
        case WLF_LOG_DEBUG:
            fprintf(stderr, "DEBUG ");
            break;
        case WLF_LOG_INFO:
            fprintf(stderr, " INFO ");
            break;
    }
    fprintf(stderr, "| %s %s:%i ", info->file, info->func, info->line);
    vfprintf(stderr, fmt, ap);
}

void
wlf_set_log_func(const wlf_log_func_t func)
{
    g_log_func = func;
}

void
wlf_set_log_level(enum wlf_log_level level)
{
    g_log_level = level;
}

void
wlf_log(enum wlf_log_level level, const char *file, const char *func, int line, const char *fmt, ...)
{
    if (g_log_func == nullptr) {
        return;
    }

    struct wlf_log_info info = {
        .level = level,
        .file = file,
        .func = func,
        .line = line,
    };

    va_list args;
    va_start(args, fmt);
    g_log_func(&info, fmt, args);
    va_end(args);
}
