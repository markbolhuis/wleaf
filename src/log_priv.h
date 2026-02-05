#pragma once

#include "wlf/log.h"

[[gnu::format(printf, 5, 6)]]
void
wlf_log(enum wlf_log_level level, const char *file, const char *func, int line, const char *fmt, ...);

#define wlf_error(fmt, ...) wlf_log(WLF_LOG_ERROR, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define wlf_warn(fmt, ...) wlf_log(WLF_LOG_WARNING, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define wlf_debug(fmt, ...) wlf_log(WLF_LOG_DEBUG, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define wlf_info(fmt, ...) wlf_log(WLF_LOG_INFO, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
