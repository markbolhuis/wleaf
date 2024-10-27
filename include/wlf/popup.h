#pragma once

#include "common.h"

struct wlf_popup;

struct wlf_popup_listener {
    void (*done)(void *user_data);
    void (*configure)(void *user_data, struct wlf_extent extent);
};

enum wlf_popup_flags : uint32_t {
    WLF_POPUP_FLAGS_NONE = 0,
    WLF_POPUP_FLAGS_REACTIVE = 1,
    // WLF_POPUP_FLAGS_INHIBIT_IDLING = 2,
};

struct wlf_popup_position {
    struct wlf_extent extent;
    struct wlf_offset offset;
    struct wlf_rect anchor_rect;
    enum wlf_edge anchor_edge;
    enum wlf_edge gravity;
    enum wlf_adjust adjust;
};

struct wlf_popup_info {
    enum wlf_popup_flags flags;
    struct wlf_popup_position position;
    struct wlf_surface *parent;
    void *user_data;
};

enum wlf_result
wlf_popup_create(
    struct wlf_context *context,
    const struct wlf_popup_info *info,
    const struct wlf_popup_listener *listener,
    struct wlf_popup **popup);

void
wlf_popup_destroy(struct wlf_popup *popup);

struct wlf_surface *
wlf_popup_get_surface(struct wlf_popup *popup);

struct wlf_popup *
wlf_popup_from_surface(struct wlf_surface *surface);

enum wlf_result
wlf_popup_reposition(
    struct wlf_popup *popup,
    const struct wlf_popup_position *position,
    bool reactive);

enum wlf_result
wlf_popup_grab(struct wlf_popup *popup, struct wlf_seat *seat, uint32_t serial);
