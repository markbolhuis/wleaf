#include <stdlib.h>
#include <assert.h>

#include <wayland-client-protocol.h>
#include <wayland-egl-core.h>
#include <viewporter-client-protocol.h>
#include <fractional-scale-v1-client-protocol.h>
#include <content-type-v1-client-protocol.h>
#include <idle-inhibit-unstable-v1-client-protocol.h>
#include <xdg-shell-client-protocol.h>
#include <xdg-decoration-unstable-v1-client-protocol.h>
#include <xdg-dialog-v1-client-protocol.h>

#include "common_priv.h"
#include "context_priv.h"
#include "input_priv.h"
#include "toplevel_priv.h"

[[maybe_unused]]
static struct wlf_rect
wlf_toplevel_get_geometry(struct wlf_toplevel *tl)
{
    return (struct wlf_rect) {
        .offset = { 0, 0 },
        .extent = tl->current.extent,
    };
}

static void
wlf_toplevel_configure(struct wlf_toplevel *tl, uint32_t serial)
{
    enum wlf_toplevel_event mask = tl->events;

    if (mask & WLF_TOPLEVEL_EVENT_CAPABILITIES) {
        tl->current.capabilities = tl->pending.capabilities;
    }

    if (mask & WLF_TOPLEVEL_EVENT_DECORATION) {
        tl->current.decoration = tl->pending.decoration;
    }

    if (mask & WLF_TOPLEVEL_EVENT_STATE) {
        tl->current.state = tl->pending.state;
    }

    if (mask & WLF_TOPLEVEL_EVENT_BOUNDS) {
        tl->current.bounds = tl->pending.bounds;
    }

    bool resized = false;
    if (mask & WLF_TOPLEVEL_EVENT_EXTENT) {
        if (tl->pending.extent.width == 0) {
            tl->pending.extent.width = tl->current.extent.width;
        }
        if (tl->pending.extent.height == 0) {
            tl->pending.extent.height = tl->current.extent.height;
        }

        resized = !wlf_extent_equal(tl->pending.extent, tl->current.extent);
        tl->current.extent = tl->pending.extent;
    }

    bool rescaled = false;
    if (mask & WLF_TOPLEVEL_EVENT_SCALE) {
        tl->current.scale = tl->pending.scale;
        rescaled = true;
        resized = true;
    }

    bool transformed = false;
    if (mask & WLF_TOPLEVEL_EVENT_TRANSFORM) {
        resized |= wlf_transform_is_perpendicular(
            tl->current.transform,
            tl->pending.transform);
        tl->current.transform = tl->pending.transform;
        transformed = true;
    }

    // TODO: Temporary
    tl->s.scale = tl->current.scale;
    tl->s.transform = tl->current.transform;
    tl->s.extent = tl->current.extent;

    tl->events = WLF_TOPLEVEL_EVENT_NONE;

    if (resized || !tl->configured) {
        struct wlf_extent se = wlf_surface_get_extent(&tl->s);
        struct wlf_extent be = wlf_surface_get_buffer_extent(&tl->s);

        if (tl->s.wl_egl_window) {
            wl_egl_window_resize(tl->s.wl_egl_window, be.width, be.height, 0, 0);
        }

        if (tl->s.wp_fractional_scale_v1) {
            assert(tl->s.wp_viewport);
            wp_viewport_set_destination(tl->s.wp_viewport, se.width, se.height);
        }

        tl->listener.configure(tl->s.user_data, be);
    }

    uint32_t version = wl_surface_get_version(tl->s.wl_surface);
    if (rescaled && !tl->s.wp_fractional_scale_v1) {
        assert(version >= WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION);
        wl_surface_set_buffer_scale(tl->s.wl_surface, tl->current.scale);
    }

    if (transformed) {
        assert(version >= WL_SURFACE_SET_BUFFER_TRANSFORM_SINCE_VERSION);
        wl_surface_set_buffer_transform(tl->s.wl_surface, tl->current.transform);
    }
}

// region XDG Toplevel Decoration

static void
xdg_toplevel_decoration_v1_configure(
    void *data,
    struct zxdg_toplevel_decoration_v1 *,
    uint32_t mode)
{
    struct wlf_toplevel *toplevel = data;

    toplevel->events &= ~WLF_TOPLEVEL_EVENT_DECORATION;

    if (toplevel->current.decoration != mode) {
        toplevel->events |= WLF_TOPLEVEL_EVENT_DECORATION;
        toplevel->pending.decoration = mode;
    }
}

static const struct zxdg_toplevel_decoration_v1_listener xdg_toplevel_decoration_v1_listener = {
    .configure = xdg_toplevel_decoration_v1_configure,
};

// endregion

// region XDG Toplevel

static void
xdg_toplevel_configure(
    void *data,
    struct xdg_toplevel *,
    int32_t width,
    int32_t height,
    struct wl_array *states)
{
    struct wlf_toplevel *toplevel = data;

    enum wlf_toplevel_state pending = WLF_TOPLEVEL_STATE_NONE;
    uint32_t *state = nullptr;
    wl_array_for_each(state, states) {
        switch (*state) {
            case XDG_TOPLEVEL_STATE_MAXIMIZED:
                pending |= WLF_TOPLEVEL_STATE_MAXIMIZED;
                break;
            case XDG_TOPLEVEL_STATE_FULLSCREEN:
                pending |= WLF_TOPLEVEL_STATE_FULLSCREEN;
                break;
            case XDG_TOPLEVEL_STATE_RESIZING:
                pending |= WLF_TOPLEVEL_STATE_RESIZING;
                break;
            case XDG_TOPLEVEL_STATE_ACTIVATED:
                pending |= WLF_TOPLEVEL_STATE_ACTIVATED;
                break;
            case XDG_TOPLEVEL_STATE_TILED_LEFT:
                pending |= WLF_TOPLEVEL_STATE_TILED_LEFT;
                break;
            case XDG_TOPLEVEL_STATE_TILED_RIGHT:
                pending |= WLF_TOPLEVEL_STATE_TILED_RIGHT;
                break;
            case XDG_TOPLEVEL_STATE_TILED_TOP:
                pending |= WLF_TOPLEVEL_STATE_TILED_TOP;
                break;
            case XDG_TOPLEVEL_STATE_TILED_BOTTOM:
                pending |= WLF_TOPLEVEL_STATE_TILED_BOTTOM;
                break;
            case XDG_TOPLEVEL_STATE_SUSPENDED:
                pending |= WLF_TOPLEVEL_STATE_SUSPENDED;
                break;
            default:
                break;
        }
    }

    toplevel->events &= ~(WLF_TOPLEVEL_EVENT_EXTENT | WLF_TOPLEVEL_EVENT_STATE);

    if (toplevel->current.extent.width != width ||
        toplevel->current.extent.height != height)
    {
        toplevel->events |= WLF_TOPLEVEL_EVENT_EXTENT;
        toplevel->pending.extent.width = width;
        toplevel->pending.extent.height = height;
    }

    if (toplevel->current.state != pending) {
        toplevel->events |= WLF_TOPLEVEL_EVENT_STATE;
        toplevel->pending.state = pending;
    }
}

static void
xdg_toplevel_close(void *data, struct xdg_toplevel *)
{
    struct wlf_toplevel *toplevel = data;
    toplevel->listener.close(toplevel->s.user_data);
}

static void
xdg_toplevel_configure_bounds(
    void *data,
    struct xdg_toplevel *,
    int32_t max_width,
    int32_t max_height)
{
    struct wlf_toplevel *toplevel = data;

    toplevel->events &= ~WLF_TOPLEVEL_EVENT_BOUNDS;

    if (toplevel->current.bounds.width != max_width ||
        toplevel->current.bounds.height != max_height)
    {
        toplevel->events |= WLF_TOPLEVEL_EVENT_BOUNDS;
        toplevel->pending.bounds.width = max_width;
        toplevel->pending.bounds.height = max_height;
    }
}

static void
xdg_toplevel_wm_capabilities(void *data, struct xdg_toplevel *, struct wl_array *capabilities)
{
    struct wlf_toplevel *toplevel = data;

    enum wlf_toplevel_capabilities pending = WLF_TOPLEVEL_CAPABILITIES_NONE;
    uint32_t *cap = nullptr;
    wl_array_for_each(cap, capabilities) {
        switch (*cap) {
            case XDG_TOPLEVEL_WM_CAPABILITIES_WINDOW_MENU:
                pending |= WLF_TOPLEVEL_CAPABILITIES_WINDOW_MENU;
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_MAXIMIZE:
                pending |= WLF_TOPLEVEL_CAPABILITIES_MAXIMIZE;
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN:
                pending |= WLF_TOPLEVEL_CAPABILITIES_FULLSCREEN;
                break;
            case XDG_TOPLEVEL_WM_CAPABILITIES_MINIMIZE:
                pending |= WLF_TOPLEVEL_CAPABILITIES_MINIMIZE;
                break;
            default:
                break;
        }
    }

    toplevel->events &= ~WLF_TOPLEVEL_EVENT_CAPABILITIES;

    if (toplevel->current.capabilities != pending) {
        toplevel->events |= WLF_TOPLEVEL_EVENT_CAPABILITIES;
        toplevel->pending.capabilities = pending;
    }
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure        = xdg_toplevel_configure,
    .close            = xdg_toplevel_close,
    .configure_bounds = xdg_toplevel_configure_bounds,
    .wm_capabilities  = xdg_toplevel_wm_capabilities,
};

// endregion

// region XDG Surface

static void
xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
    struct wlf_toplevel *toplevel = data;

    if (toplevel->events != WLF_TOPLEVEL_EVENT_NONE || !toplevel->configured) {
        wlf_toplevel_configure(toplevel, serial);
    }

    xdg_surface_ack_configure(toplevel->xdg_surface, serial);
    toplevel->configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

// endregion

// region Surface

static void
wlf_toplevel_configure_scale(struct wlf_surface *surface, int32_t scale)
{
    struct wlf_toplevel *tl = wl_container_of(surface, tl, s);

    tl->events &= ~WLF_TOPLEVEL_EVENT_SCALE;

    if (tl->current.scale != scale) {
        tl->events |= WLF_TOPLEVEL_EVENT_SCALE;
        tl->pending.scale = scale;

        if (tl->configured) {
            wlf_toplevel_configure(tl, 0);
        }
    }
}

static void
wlf_toplevel_configure_transform(struct wlf_surface *surface, enum wlf_transform transform)
{
    struct wlf_toplevel *tl = wl_container_of(surface, tl, s);

    tl->events &= ~WLF_TOPLEVEL_EVENT_TRANSFORM;

    if (tl->current.transform != transform) {
        tl->events |= WLF_TOPLEVEL_EVENT_TRANSFORM;
        tl->pending.transform = transform;

        if (tl->configured) {
            wlf_toplevel_configure(tl, 0);
        }
    }
}

// endregion

static void
wlf_toplevel_init_state(struct wlf_toplevel *toplevel, const struct wlf_toplevel_info *info)
{
    if (toplevel->s.wp_fractional_scale_v1) {
        toplevel->current.scale = 120;
    } else {
        toplevel->current.scale = 1;
    }
    toplevel->current.transform = WLF_TRANSFORM_NONE;
    toplevel->current.extent = info->extent;
    toplevel->current.state = WLF_TOPLEVEL_STATE_NONE;
    toplevel->current.bounds.width = 0;
    toplevel->current.bounds.height = 0;
    toplevel->current.capabilities = WLF_TOPLEVEL_CAPABILITIES_NONE;
    toplevel->current.decoration = WLF_DECORATION_MODE_NO_PREFERENCE;

    if (xdg_toplevel_get_version(toplevel->xdg_toplevel) <
        XDG_TOPLEVEL_WM_CAPABILITIES_SINCE_VERSION)
    {
        toplevel->events |= WLF_TOPLEVEL_EVENT_CAPABILITIES;
        toplevel->pending.capabilities = WLF_TOPLEVEL_CAPABILITIES_WINDOW_MENU
                                       | WLF_TOPLEVEL_CAPABILITIES_MAXIMIZE
                                       | WLF_TOPLEVEL_CAPABILITIES_FULLSCREEN
                                       | WLF_TOPLEVEL_CAPABILITIES_MINIMIZE;
    }

    if (!toplevel->xdg_toplevel_decoration_v1) {
        toplevel->events |= WLF_TOPLEVEL_EVENT_DECORATION;
        toplevel->pending.decoration = WLF_DECORATION_MODE_CLIENT_SIDE;
    }

    if (info->app_id) {
        wlf_toplevel_set_app_id(toplevel, info->app_id);
    }

    if (info->title) {
        wlf_toplevel_set_title(toplevel, info->title);
    }

    if (info->parent) {
        wlf_toplevel_set_parent(toplevel, info->parent);
    }

    if (info->decoration_mode != WLF_DECORATION_MODE_NO_PREFERENCE &&
        toplevel->xdg_toplevel_decoration_v1)
    {
        wlf_toplevel_set_decoration_mode(toplevel, info->decoration_mode);
    }

    if (info->content_type != WLF_CONTENT_TYPE_NONE && toplevel->s.wp_content_type_v1) {
        wlf_surface_set_content_type(&toplevel->s, info->content_type);
    }

    if (info->flags & WLF_TOPLEVEL_FLAGS_INHIBIT_IDLING && toplevel->s.wp_idle_inhibitor_v1) {
        wlf_surface_inhibit_idling(&toplevel->s, true);
    }
}

static enum wlf_result
wlf_toplevel_init(
    struct wlf_context *context,
    const struct wlf_toplevel_info *info,
    const struct wlf_toplevel_listener *listener,
    struct wlf_toplevel *toplevel)
{
    enum wlf_result res = wlf_surface_init(context, WLF_SURFACE_TYPE_TOPLEVEL, &toplevel->s);
    if (res < WLF_SUCCESS) {
        return res;
    }

    toplevel->s.configure_scale = wlf_toplevel_configure_scale,
    toplevel->s.configure_transform = wlf_toplevel_configure_transform,
    toplevel->s.user_data = info->user_data;
    toplevel->listener = *listener;

    toplevel->xdg_surface = xdg_wm_base_get_xdg_surface(
        context->xdg_wm_base, toplevel->s.wl_surface);
    xdg_surface_add_listener(toplevel->xdg_surface, &xdg_surface_listener, toplevel);

    toplevel->xdg_toplevel = xdg_surface_get_toplevel(toplevel->xdg_surface);
    xdg_toplevel_add_listener(toplevel->xdg_toplevel, &xdg_toplevel_listener, toplevel);

    if (context->xdg_decoration_manager_v1) {
        toplevel->xdg_toplevel_decoration_v1 = zxdg_decoration_manager_v1_get_toplevel_decoration(
            context->xdg_decoration_manager_v1,
            toplevel->xdg_toplevel);
        zxdg_toplevel_decoration_v1_add_listener(
            toplevel->xdg_toplevel_decoration_v1,
            &xdg_toplevel_decoration_v1_listener,
            toplevel);
    }

    if ((info->flags & WLF_TOPLEVEL_FLAGS_DIALOG) && context->xdg_wm_dialog_v1) {
        toplevel->xdg_dialog_v1 = xdg_wm_dialog_v1_get_xdg_dialog(
            context->xdg_wm_dialog_v1,
            toplevel->xdg_toplevel);
    }

    wlf_toplevel_init_state(toplevel, info);
    wl_surface_commit(toplevel->s.wl_surface);

    return WLF_SUCCESS;
}

static void
wlf_toplevel_fini(struct wlf_toplevel *toplevel)
{
    if (toplevel->xdg_toplevel_decoration_v1) {
        zxdg_toplevel_decoration_v1_destroy(toplevel->xdg_toplevel_decoration_v1);
    }
    if (toplevel->xdg_dialog_v1) {
        xdg_dialog_v1_destroy(toplevel->xdg_dialog_v1);
    }
    xdg_toplevel_destroy(toplevel->xdg_toplevel);
    xdg_surface_destroy(toplevel->xdg_surface);
    wlf_surface_fini(&toplevel->s);
}

// region Public API

enum wlf_result
wlf_toplevel_create(
    struct wlf_context *context,
    const struct wlf_toplevel_info *info,
    const struct wlf_toplevel_listener *listener,
    struct wlf_toplevel **_toplevel)
{
    if (!context->xdg_wm_base) {
        return WLF_ERROR_UNSUPPORTED;
    }

    struct wlf_toplevel *toplevel = calloc(1, sizeof(struct wlf_toplevel));
    if (!toplevel) {
        return WLF_ERROR_OUT_OF_MEMORY;
    }

    enum wlf_result result = wlf_toplevel_init(context, info, listener, toplevel);
    if (result < WLF_SUCCESS) {
        free(toplevel);
    } else {
        *_toplevel = toplevel;
    }

    return result;
}

void
wlf_toplevel_destroy(struct wlf_toplevel *toplevel)
{
    wlf_toplevel_fini(toplevel);
    free(toplevel);
}

struct wlf_surface *
wlf_toplevel_get_surface(struct wlf_toplevel *toplevel)
{
    return &toplevel->s;
}

struct wlf_toplevel *
wlf_toplevel_from_surface(struct wlf_surface *surface)
{
    struct wlf_toplevel *t = nullptr;
    if (surface->type == WLF_SURFACE_TYPE_TOPLEVEL) {
        t = wl_container_of(surface, t, s);
    }
    return t;
}

void
wlf_toplevel_set_parent(struct wlf_toplevel *toplevel, struct wlf_toplevel *parent)
{
    struct xdg_toplevel *xdg_parent = nullptr;
    if (parent) {
        xdg_parent = parent->xdg_toplevel;
    }
    xdg_toplevel_set_parent(toplevel->xdg_toplevel, xdg_parent);
}

void
wlf_toplevel_set_title(struct wlf_toplevel *toplevel, const char8_t *title)
{
    xdg_toplevel_set_title(toplevel->xdg_toplevel, title ? (const char *)title : "");
}

void
wlf_toplevel_set_app_id(struct wlf_toplevel *toplevel, const char8_t *app_id)
{
    xdg_toplevel_set_app_id(toplevel->xdg_toplevel, app_id ? (const char *)app_id : "");
}

enum wlf_result
wlf_toplevel_move(struct wlf_toplevel *toplevel, struct wlf_seat *seat, uint32_t serial)
{
    if (!seat->wl_seat) {
        return WLF_ERROR_LOST;
    }
    xdg_toplevel_move(toplevel->xdg_toplevel, seat->wl_seat, serial);
    return WLF_SUCCESS;
}

enum wlf_result
wlf_toplevel_resize(struct wlf_toplevel *toplevel, struct wlf_seat *seat, uint32_t serial, enum wlf_edge edge)
{
    edge = wlf_edge_clean(edge);
    if (edge == WLF_EDGE_NONE) {
        return WLF_ERROR_INVALID_ARGUMENT;
    }
    if (!seat->wl_seat) {
        return WLF_ERROR_LOST;
    }
    xdg_toplevel_resize(toplevel->xdg_toplevel, seat->wl_seat, serial, edge);
    return WLF_SUCCESS;
}

enum wlf_result
wlf_toplevel_set_min_extent(struct wlf_toplevel *toplevel, struct wlf_extent extent)
{
    if (extent.width < 0 || extent.height < 0) {
        return WLF_ERROR_INVALID_ARGUMENT;
    }
    xdg_toplevel_set_min_size(toplevel->xdg_toplevel, extent.width, extent.height);
    return WLF_SUCCESS;
}

enum wlf_result
wlf_toplevel_set_max_extent(struct wlf_toplevel *toplevel, struct wlf_extent extent)
{
    if (extent.width < 0 || extent.height < 0) {
        return WLF_ERROR_INVALID_ARGUMENT;
    }
    xdg_toplevel_set_max_size(toplevel->xdg_toplevel, extent.width, extent.height);
    return WLF_SUCCESS;
}

enum wlf_result
wlf_toplevel_show_menu(struct wlf_toplevel *toplevel, struct wlf_seat *seat, uint32_t serial, struct wlf_offset offset)
{
    if (!seat->wl_seat) {
        return WLF_ERROR_LOST;
    }

    if ((toplevel->current.capabilities & WLF_TOPLEVEL_CAPABILITIES_WINDOW_MENU) == 0) {
        return WLF_SKIPPED;
    }

    xdg_toplevel_show_window_menu(
        toplevel->xdg_toplevel,
        seat->wl_seat,
        serial,
        offset.x,
        offset.y);

    return WLF_SUCCESS;
}

enum wlf_result
wlf_toplevel_set_fullscreen(struct wlf_toplevel *toplevel, bool set)
{
    if ((toplevel->current.capabilities & WLF_TOPLEVEL_CAPABILITIES_FULLSCREEN) == 0) {
        return WLF_SKIPPED;
    }
    if (set) {
        xdg_toplevel_set_fullscreen(toplevel->xdg_toplevel, nullptr);
    } else {
        xdg_toplevel_unset_fullscreen(toplevel->xdg_toplevel);
    }
    return WLF_SUCCESS;
}

enum wlf_result
wlf_toplevel_set_maximized(struct wlf_toplevel *toplevel, bool set)
{
    if ((toplevel->current.capabilities & WLF_TOPLEVEL_CAPABILITIES_MAXIMIZE) == 0) {
        return WLF_SKIPPED;
    }
    if (set) {
        xdg_toplevel_set_maximized(toplevel->xdg_toplevel);
    } else {
        xdg_toplevel_unset_maximized(toplevel->xdg_toplevel);
    }
    return WLF_SUCCESS;
}

enum wlf_result
wlf_toplevel_set_minimized(struct wlf_toplevel *toplevel)
{
    if ((toplevel->current.capabilities & WLF_TOPLEVEL_CAPABILITIES_MINIMIZE) == 0) {
        return WLF_SKIPPED;
    }
    xdg_toplevel_set_minimized(toplevel->xdg_toplevel);
    return WLF_SUCCESS;
}

enum wlf_result
wlf_toplevel_set_decoration_mode(struct wlf_toplevel *toplevel, enum wlf_decoration_mode mode)
{
    if (!toplevel->xdg_toplevel_decoration_v1) {
        return WLF_ERROR_UNSUPPORTED;
    }

    switch (mode) {
        case WLF_DECORATION_MODE_NO_PREFERENCE:
            zxdg_toplevel_decoration_v1_unset_mode(toplevel->xdg_toplevel_decoration_v1);
            break;
        case WLF_DECORATION_MODE_CLIENT_SIDE:
        case WLF_DECORATION_MODE_SERVER_SIDE:
            zxdg_toplevel_decoration_v1_set_mode(toplevel->xdg_toplevel_decoration_v1, mode);
            break;
        default:
            return WLF_ERROR_INVALID_ARGUMENT;
    }

    return WLF_SUCCESS;
}

enum wlf_result
wlf_toplevel_set_dialog_modal(struct wlf_toplevel *toplevel, bool set)
{
    if (!toplevel->xdg_dialog_v1) {
        return WLF_ERROR_UNSUPPORTED;
    }
    if (set) {
        xdg_dialog_v1_set_modal(toplevel->xdg_dialog_v1);
    } else {
        xdg_dialog_v1_unset_modal(toplevel->xdg_dialog_v1);
    }
    return WLF_SUCCESS;
}

// endregion
