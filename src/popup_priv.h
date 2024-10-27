#pragma once

#include "wlf/popup.h"
#include "surface_priv.h"

struct wlf_popup {
    struct wlf_surface s;
    struct wlf_popup_listener listener;

    struct xdg_surface *xdg_surface;
    struct xdg_popup   *xdg_popup;

    struct {
        struct wlf_offset offset;
        uint32_t token;
    } current, pending;
};
