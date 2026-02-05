#pragma once

#include "common.h"

#include <stdarg.h>

enum wlf_log_level {
    WLF_LOG_ERROR = 0,
    WLF_LOG_WARNING = 1,
    WLF_LOG_INFO = 3,
    WLF_LOG_DEBUG = 4,
};

struct wlf_log_info {
    enum wlf_log_level level;
    const char *file;
    const char *func;
    int line;
};

typedef void (*wlf_log_func_t)(
    const struct wlf_log_info *info,
    const char *fmt,
    va_list ap);

void
wlf_set_log_func(wlf_log_func_t func);

void
wlf_set_log_level(enum wlf_log_level level);
