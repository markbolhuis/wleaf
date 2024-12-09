#pragma once

#include <wlf/surface.h>

struct wlf_output;

enum wlf_surface_type : uint32_t {
    WLF_SURFACE_TYPE_TOPLEVEL = 1,
    WLF_SURFACE_TYPE_POPUP = 2,
};

struct wlf_surface {
    enum wlf_surface_type type;
    struct wlf_context *context;

    struct wlf_extent extent;
    int32_t scale;
    enum wlf_transform transform;

    struct wl_surface                   *wl_surface;
    struct wl_egl_window                *wl_egl_window;
    struct wp_viewport                  *wp_viewport;
    struct wp_fractional_scale_v1       *wp_fractional_scale_v1;
    struct wp_content_type_v1           *wp_content_type_v1;
    struct zwp_idle_inhibitor_v1        *wp_idle_inhibitor_v1;
    struct wp_alpha_modifier_surface_v1 *wp_alpha_modifier_surface_v1;

    struct wl_list link;
    struct wl_list output_list;

    // void (*enter)(struct wlf_surface *surface, struct wlf_output *output);
    // void (*leave)(struct wlf_surface *surface, struct wlf_output *output);

    void (*configure_scale)(struct wlf_surface *surface, int32_t scale);
    void (*configure_transform)(struct wlf_surface *surface, enum wlf_transform transform);

    void *user_data;
};

void
wlf_surface_handle_output_destroyed(struct wlf_surface *s, struct wlf_output *o);

struct wlf_extent
wlf_surface_get_extent(struct wlf_surface *surface);

struct wlf_extent
wlf_surface_get_buffer_extent(struct wlf_surface *surface);

enum wlf_result
wlf_surface_init(struct wlf_context *context, enum wlf_surface_type type, struct wlf_surface *surface);

void
wlf_surface_fini(struct wlf_surface *surface);

struct wl_egl_window *
wlf_surface_create_egl_window(struct wlf_surface *surface);

void
wlf_surface_destroy_egl_window(struct wlf_surface *surface);
