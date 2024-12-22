#pragma once

#include "common.h"

struct wlf_toplevel_listener {
    void (*close)(void *user_data);
    void (*configure)(void *user_data, struct wlf_extent extent);
};

enum wlf_toplevel_flags : uint32_t {
    WLF_TOPLEVEL_FLAGS_NONE = 0,
    WLF_TOPLEVEL_FLAGS_DIALOG = 1,
    WLF_TOPLEVEL_FLAGS_INHIBIT_IDLING = 2,
    // WLF_TOPLEVEL_FLAGS_VIEWPORT_RESIZE = 4,
    // WLF_TOPLEVEL_FLAGS_MAINTAIN_ASPECT_RATIO = 8,
    // WLF_TOPLEVEL_FLAGS_FIXED_BUFFER_EXTENT = 16,
};

enum wlf_decoration_mode : uint32_t {
    WLF_DECORATION_MODE_NO_PREFERENCE = 0,
    WLF_DECORATION_MODE_CLIENT_SIDE = 1,
    WLF_DECORATION_MODE_SERVER_SIDE = 2,
};

struct wlf_toplevel_info {
    enum wlf_toplevel_flags  flags;
    struct wlf_extent        extent;
    enum wlf_decoration_mode decoration_mode;
    enum wlf_content_type    content_type;
    struct wlf_toplevel      *parent;
    const char8_t            *app_id;
    const char8_t            *title;
    bool                     modal;
    void                     *user_data;
};

enum wlf_result
wlf_toplevel_create(
    struct wlf_context *context,
    const struct wlf_toplevel_info *info,
    const struct wlf_toplevel_listener *listener,
    struct wlf_toplevel **toplevel);

void
wlf_toplevel_destroy(struct wlf_toplevel *toplevel);

struct wlf_surface *
wlf_toplevel_get_surface(struct wlf_toplevel *toplevel);

struct wlf_toplevel *
wlf_toplevel_from_surface(struct wlf_surface *surface);

void
wlf_toplevel_set_parent(struct wlf_toplevel *toplevel, struct wlf_toplevel *parent);

void
wlf_toplevel_set_app_id(struct wlf_toplevel *toplevel, const char8_t *app_id);

void
wlf_toplevel_set_title(struct wlf_toplevel *toplevel, const char8_t *title);

enum wlf_result
wlf_toplevel_move(struct wlf_toplevel *toplevel, struct wlf_seat *seat, uint32_t serial);

enum wlf_result
wlf_toplevel_resize(struct wlf_toplevel *toplevel, struct wlf_seat *seat, uint32_t serial, enum wlf_edge edge);

enum wlf_result
wlf_toplevel_set_min_extent(struct wlf_toplevel *toplevel, struct wlf_extent extent);

enum wlf_result
wlf_toplevel_set_max_extent(struct wlf_toplevel *toplevel, struct wlf_extent extent);

enum wlf_result
wlf_toplevel_show_menu(struct wlf_toplevel *toplevel, struct wlf_seat *seat, uint32_t serial, struct wlf_offset offset);

enum wlf_result
wlf_toplevel_set_fullscreen(struct wlf_toplevel *toplevel, bool set);

enum wlf_result
wlf_toplevel_set_maximized(struct wlf_toplevel *toplevel, bool set);

enum wlf_result
wlf_toplevel_set_minimized(struct wlf_toplevel *toplevel);

enum wlf_result
wlf_toplevel_set_decoration_mode(struct wlf_toplevel *toplevel, enum wlf_decoration_mode mode);

enum wlf_result
wlf_toplevel_set_dialog_modal(struct wlf_toplevel *toplevel, bool set);
