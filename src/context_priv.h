#pragma once

#include "wlf/context.h"

struct wlf_global {
    struct wlf_context *context;
    uint64_t id;
    uint32_t name;
    uint32_t version;
};

struct wlf_context {
    struct wlf_context_listener listener;

    struct wl_display *wl_display;
    struct wl_registry *wl_registry;

    struct wl_list seat_list;
    struct wl_list output_list;
    struct wl_list surface_list;
    struct wl_array format_array;

    struct wl_compositor                             *wl_compositor;
    struct wl_subcompositor                          *wl_subcompositor;
    struct wl_shm                                    *wl_shm;
    struct wl_data_device_manager                    *wl_data_device_manager;
    struct wp_viewporter                             *wp_viewporter;
    struct wp_fractional_scale_manager_v1            *wp_fractional_scale_manager_v1;
    struct wp_single_pixel_buffer_manager_v1         *wp_single_pixel_buffer_manager_v1;
    struct zwp_input_timestamps_manager_v1           *wp_input_timestamps_manager_v1;
    struct zwp_relative_pointer_manager_v1           *wp_relative_pointer_manager_v1;
    struct zwp_pointer_constraints_v1                *wp_pointer_constraints_v1;
    struct zwp_pointer_gestures_v1                   *wp_pointer_gestures_v1;
    struct zwp_keyboard_shortcuts_inhibit_manager_v1 *wp_keyboard_shortcuts_inhibit_manager_v1;
    struct zwp_text_input_manager_v3                 *wp_text_input_manager_v3;
    struct zwp_idle_inhibit_manager_v1               *wp_idle_inhibit_manager_v1;
    struct wp_content_type_manager_v1                *wp_content_type_manager_v1;
    struct wp_alpha_modifier_v1                      *wp_alpha_modifier_v1;
    struct xdg_wm_base                               *xdg_wm_base;
    struct zxdg_decoration_manager_v1                *xdg_decoration_manager_v1;
    struct zxdg_output_manager_v1                    *xdg_output_manager_v1;
    struct xdg_wm_dialog_v1                          *xdg_wm_dialog_v1;
    struct ext_idle_notifier_v1                      *ext_idle_notifier_v1;

    struct xkb_context *xkb_context;

    void *user_data;
};

uint64_t
wlf_new_id();

uint32_t
wlf_get_version(const struct wl_interface *interface, uint32_t version, uint32_t max);
