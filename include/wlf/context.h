#pragma once

#include "wlf/common.h"

struct wlf_context_listener {
    void (*seat_added)(void *user_data, uint64_t id);
};

enum wlf_context_flags : uint32_t {
    WLF_CONTEXT_FLAGS_NONE = 0,
    // WLF_CONTEXT_FLAGS_DBUS = 1,
};

struct wlf_context_info {
    enum wlf_context_flags flags;
    const char *display;
    void *user_data;
};

enum wlf_result
wlf_context_create(
    const struct wlf_context_info *info,
    const struct wlf_context_listener *listener,
    struct wlf_context **context);

void
wlf_context_destroy(struct wlf_context *context);

enum wlf_result
wlf_dispatch_events(struct wlf_context *context, int64_t timeout);
