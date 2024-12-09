#include <malloc.h>

#include <wayland-client-protocol.h>
#include <wayland-egl-core.h>
#include <viewporter-client-protocol.h>
#include <content-type-v1-client-protocol.h>
#include <fractional-scale-v1-client-protocol.h>
#include <idle-inhibit-unstable-v1-client-protocol.h>
#include <alpha-modifier-v1-client-protocol.h>

#include "common_priv.h"
#include "context_priv.h"
#include "input_priv.h"
#include "output_priv.h"
#include "surface_priv.h"

struct wlf_extent
wlf_surface_get_extent(struct wlf_surface *surface)
{
    return surface->extent;
}

struct wlf_extent
wlf_surface_get_buffer_extent(struct wlf_surface *surface)
{
    struct wlf_extent extent = wlf_surface_get_extent(surface);
    if (wlf_transform_is_vertical(surface->transform)) {
        int32_t tmp = extent.width;
        extent.width = extent.height;
        extent.height = tmp;
    }
    extent.width *= surface->scale;
    extent.height *= surface->scale;
    if (surface->wp_fractional_scale_v1) {
        extent.width = wlf_div_round(extent.width, 120);
        extent.height = wlf_div_round(extent.height, 120);
    }
    return extent;
}

static void
wlf_surface_update_output(struct wlf_surface *surface, struct wlf_output *output, bool added)
{
    uint32_t version = wl_surface_get_version(surface->wl_surface);
    struct wl_list *list = &surface->output_list;

    bool updated = added
                 ? wlf_output_ref_list_add(list, output)
                 : wlf_output_ref_list_remove(list, output);

    if (!updated ||
        version < WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION ||
#ifdef WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION
        version >= WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION ||
#endif
        surface->wp_fractional_scale_v1)
    {
        return;
    }

    int32_t scale = wlf_output_ref_list_get_max_scale(list);
    surface->configure_scale(surface, scale);
}

void
wlf_surface_handle_output_destroyed(struct wlf_surface *s, struct wlf_output *o)
{
    // Some compositors like KWin send a wl_surface.leave event before destroying
    // the wl_output object. Others like Sway do not, so the surface scale needs to be
    // updated when the wl_output object is destroyed.
    wlf_surface_update_output(s, o, false);
}

// region WP Fractional Scale

static void
wp_fractional_scale_v1_preferred_scale(void *data, struct wp_fractional_scale_v1 *, uint32_t scale)
{
    struct wlf_surface *surface = data;
    surface->configure_scale(surface, (int32_t)scale);
}

static const struct wp_fractional_scale_v1_listener wp_fractional_scale_listener = {
    .preferred_scale = wp_fractional_scale_v1_preferred_scale,
};

// endregion

// region WL Surface

static void
wl_surface_enter(void *data, struct wl_surface *, struct wl_output *wl_output)
{
    if (!wl_output) {
        return;
    }
    struct wlf_output *output = wl_output_get_user_data(wl_output);
    wlf_surface_update_output(data, output, true);
}

static void
wl_surface_leave(void *data, struct wl_surface *, struct wl_output *wl_output)
{
    if (!wl_output) {
        return;
    }
    struct wlf_output *output = wl_output_get_user_data(wl_output);
    wlf_surface_update_output(data, output, false);
}

#ifdef WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION
static void
wl_surface_preferred_buffer_scale(void *data, struct wl_surface *, int32_t factor)
{
    struct wlf_surface *surface = data;

    if (surface->wp_fractional_scale_v1) {
        return;
    }

    surface->configure_scale(surface, factor);
}
#endif

#ifdef WL_SURFACE_PREFERRED_BUFFER_TRANSFORM_SINCE_VERSION
static void
wl_surface_preferred_buffer_transform(void *data, struct wl_surface *, uint32_t transform)
{
    struct wlf_surface *surface = data;
    surface->configure_transform(surface, transform);
}
#endif

static const struct wl_surface_listener wl_surface_listener = {
    .enter                      = wl_surface_enter,
    .leave                      = wl_surface_leave,
#ifdef WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION
    .preferred_buffer_scale     = wl_surface_preferred_buffer_scale,
#endif
#ifdef WL_SURFACE_PREFERRED_BUFFER_TRANSFORM_SINCE_VERSION
    .preferred_buffer_transform = wl_surface_preferred_buffer_transform,
#endif
};

// endregion

enum wlf_result
wlf_surface_init(
    struct wlf_context *context,
    enum wlf_surface_type type,
    struct wlf_surface *surface)
{
    surface->context = context;
    surface->type = type;

    wl_list_init(&surface->output_list);

    surface->wl_surface = wl_compositor_create_surface(context->wl_compositor);
    wl_surface_add_listener(surface->wl_surface, &wl_surface_listener, surface);

    if (context->wp_viewporter) {
        surface->wp_viewport = wp_viewporter_get_viewport(
            context->wp_viewporter,
            surface->wl_surface);

        if (context->wp_fractional_scale_manager_v1) {
            surface->wp_fractional_scale_v1 = wp_fractional_scale_manager_v1_get_fractional_scale(
                context->wp_fractional_scale_manager_v1,
                surface->wl_surface);
            wp_fractional_scale_v1_add_listener(
                surface->wp_fractional_scale_v1,
                &wp_fractional_scale_listener,
                surface);
        }
    }

    if (context->wp_content_type_manager_v1) {
        surface->wp_content_type_v1 = wp_content_type_manager_v1_get_surface_content_type(
            context->wp_content_type_manager_v1,
            surface->wl_surface);
    }

    if (context->wp_alpha_modifier_v1) {
        surface->wp_alpha_modifier_surface_v1 = wp_alpha_modifier_v1_get_surface(
            context->wp_alpha_modifier_v1,
            surface->wl_surface);
    }

    wl_list_insert(&context->surface_list, &surface->link);
    return WLF_SUCCESS;
}

void
wlf_surface_fini(struct wlf_surface *surface)
{
    wl_list_remove(&surface->link);

    struct wlf_context *context = surface->context;

    struct wlf_seat *seat;
    wl_list_for_each(seat, &context->seat_list, link) {
        wlf_seat_handle_surface_destroyed(seat, surface);
    }

    struct wlf_output_ref *ref, *ref_tmp;
    wl_list_for_each_safe(ref, ref_tmp, &surface->output_list, link) {
        wl_list_remove(&ref->link);
        free(ref);
    }

    if (surface->wp_alpha_modifier_surface_v1) {
        wp_alpha_modifier_surface_v1_destroy(surface->wp_alpha_modifier_surface_v1);
    }

    if (surface->wp_content_type_v1) {
        wp_content_type_v1_destroy(surface->wp_content_type_v1);
    }

    if (surface->wp_idle_inhibitor_v1) {
        zwp_idle_inhibitor_v1_destroy(surface->wp_idle_inhibitor_v1);
    }

    if (surface->wp_fractional_scale_v1) {
        wp_fractional_scale_v1_destroy(surface->wp_fractional_scale_v1);
    }

    if (surface->wp_viewport) {
        wp_viewport_destroy(surface->wp_viewport);
    }

    if (surface->wl_egl_window) {
        wl_egl_window_destroy(surface->wl_egl_window);
    }

    wl_surface_destroy(surface->wl_surface);
}

struct wl_egl_window *
wlf_surface_create_egl_window(struct wlf_surface *surface)
{
    if (!surface->wl_egl_window) {
        struct wlf_extent extent = wlf_surface_get_buffer_extent(surface);
        surface->wl_egl_window = wl_egl_window_create(
            surface->wl_surface,
            extent.width,
            extent.height);
    }
    return surface->wl_egl_window;
}

void
wlf_surface_destroy_egl_window(struct wlf_surface *surface)
{
    if (surface->wl_egl_window) {
        wl_egl_window_destroy(surface->wl_egl_window);
        surface->wl_egl_window = nullptr;
    }
}

// region Public API

void
wlf_surface_set_user_data(struct wlf_surface *surface, void *data)
{
    surface->user_data = data;
}

void *
wlf_surface_get_user_data(struct wlf_surface *surface)
{
    return surface->user_data;
}

enum wlf_result
wlf_surface_inhibit_idling(struct wlf_surface *surface, bool enable)
{
    struct zwp_idle_inhibit_manager_v1 *manager = surface->context->wp_idle_inhibit_manager_v1;
    if (!manager) {
        return WLF_ERROR_UNSUPPORTED;
    }

    if (enable) {
        if (surface->wp_idle_inhibitor_v1) {
            return WLF_ALREADY_SET;
        }
        surface->wp_idle_inhibitor_v1 = zwp_idle_inhibit_manager_v1_create_inhibitor(
            manager, surface->wl_surface);
        return WLF_SUCCESS;
    }

    if (!surface->wp_idle_inhibitor_v1) {
        return WLF_SKIPPED;
    }

    zwp_idle_inhibitor_v1_destroy(surface->wp_idle_inhibitor_v1);
    surface->wp_idle_inhibitor_v1 = nullptr;
    return WLF_SUCCESS;
}

enum wlf_result
wlf_surface_set_content_type(struct wlf_surface *surface, enum wlf_content_type type)
{
    if (!surface->wp_content_type_v1) {
        return WLF_ERROR_UNSUPPORTED;
    }
    wp_content_type_v1_set_content_type(surface->wp_content_type_v1, type);
    return WLF_SUCCESS;
}

enum wlf_result
wlf_surface_set_alpha_multiplier(struct wlf_surface *surface, uint32_t factor)
{
    if (!surface->wp_alpha_modifier_surface_v1) {
        return WLF_ERROR_UNSUPPORTED;
    }
    wp_alpha_modifier_surface_v1_set_multiplier(surface->wp_alpha_modifier_surface_v1, factor);
    return WLF_SUCCESS;
}

struct wlf_offset
wlf_surface_point_to_buffer_offset(struct wlf_surface *surface, struct wlf_point point)
{
    struct wlf_extent extent = wlf_surface_get_extent(surface);
    enum wlf_transform rev = wlf_transform_inverse(surface->transform);

    struct wlf_point tp = wlf_point_transform(point, extent, rev);

    double scale = surface->scale;

    if (surface->wp_fractional_scale_v1) {
        scale /= 120.0;
    }
    tp.x *= scale;
    tp.y *= scale;

    return (struct wlf_offset) {
        .x = (int32_t)tp.x,
        .y = (int32_t)tp.y,
    };
}

// endregion
