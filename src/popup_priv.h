#pragma once

#include "wlf/popup.h"
#include "surface_priv.h"

enum wlf_popup_event : uint32_t {
    WLF_POPUP_EVENT_NONE = 0,
    WLF_POPUP_EVENT_SCALE = 1,
    WLF_POPUP_EVENT_TRANSFORM = 2,
    WLF_POPUP_EVENT_OFFSET = 4,
    WLF_POPUP_EVENT_EXTENT = 8,
    WLF_POPUP_EVENT_REPOSITIONED = 16,
};

struct wlf_popup {
    struct wlf_surface s;
    struct wlf_popup_listener listener;

    struct xdg_surface *xdg_surface;
    struct xdg_popup   *xdg_popup;

    enum wlf_popup_event events;
    struct {
        struct wlf_extent extent;
        struct wlf_offset offset;
        int32_t scale;
        enum wlf_transform transform;
        uint32_t token;
    } current, pending;
    bool configured;
};
