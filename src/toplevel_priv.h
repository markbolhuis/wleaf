#pragma once

#include "wlf/toplevel.h"
#include "surface_priv.h"

enum wlf_toplevel_capabilities : uint32_t {
    WLF_TOPLEVEL_CAPABILITIES_NONE = 0,
    WLF_TOPLEVEL_CAPABILITIES_WINDOW_MENU = 1,
    WLF_TOPLEVEL_CAPABILITIES_MAXIMIZE = 2,
    WLF_TOPLEVEL_CAPABILITIES_FULLSCREEN = 4,
    WLF_TOPLEVEL_CAPABILITIES_MINIMIZE = 8,
};

enum wlf_toplevel_state : uint32_t {
    WLF_TOPLEVEL_STATE_NONE = 0,
    WLF_TOPLEVEL_STATE_MAXIMIZED = 1,
    WLF_TOPLEVEL_STATE_FULLSCREEN = 2,
    WLF_TOPLEVEL_STATE_RESIZING = 4,
    WLF_TOPLEVEL_STATE_ACTIVATED = 8,
    WLF_TOPLEVEL_STATE_TILED_LEFT = 16,
    WLF_TOPLEVEL_STATE_TILED_RIGHT = 32,
    WLF_TOPLEVEL_STATE_TILED_TOP = 64,
    WLF_TOPLEVEL_STATE_TILED_BOTTOM = 128,
    WLF_TOPLEVEL_STATE_SUSPENDED = 256,
};

struct wlf_toplevel {
    struct wlf_surface s;
    struct wlf_toplevel_listener listener;

    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct xdg_dialog_v1 *xdg_dialog_v1;
    struct zxdg_toplevel_decoration_v1 *xdg_toplevel_decoration_v1;

    struct {
        struct wlf_extent bounds;
        enum wlf_toplevel_state state;
        enum wlf_toplevel_capabilities capabilities;
        enum wlf_decoration_mode decoration;
    } pending, current;

    bool configured;
};
