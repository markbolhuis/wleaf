#include <stdlib.h>
#include <assert.h>

#include <wayland-client-protocol.h>
#include <wayland-egl-core.h>
#include <viewporter-client-protocol.h>
#include <fractional-scale-v1-client-protocol.h>
#include <xdg-shell-client-protocol.h>

#include "common_priv.h"
#include "context_priv.h"
#include "input_priv.h"
#include "toplevel_priv.h"
#include "popup_priv.h"

static uint32_t
edge_to_xdg(enum wlf_edge edge)
{
    // WLF_EDGE_NONE 0            XDG_POSITIONER_ANCHOR_NONE 0
    // WLF_EDGE_TOP 1             XDG_POSITIONER_ANCHOR_TOP 1
    // WLF_EDGE_BOTTOM 2          XDG_POSITIONER_ANCHOR_BOTTOM 2
    // WLF_EDGE_LEFT 4            XDG_POSITIONER_ANCHOR_LEFT 3
    // WLF_EDGE_TOP_LEFT 5        XDG_POSITIONER_ANCHOR_TOP_LEFT 5
    // WLF_EDGE_BOTTOM_LEFT 6     XDG_POSITIONER_ANCHOR_BOTTOM_LEFT 6
    // WLF_EDGE_RIGHT 8           XDG_POSITIONER_ANCHOR_RIGHT 4
    // WLF_EDGE_TOP_RIGHT 9       XDG_POSITIONER_ANCHOR_TOP_RIGHT 7
    // WLF_EDGE_BOTTOM_RIGHT 10   XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT 8

    uint32_t arr[16] = {
        0, 1, 2,    // none, top, bottom
        0,          // top-bottom
        3, 5, 6, 3, // left, top-left, bottom-left, top-bottom-left
        4, 7, 8, 4, // right, top-right, bottom-right, top-bottom-right
        0,          // left-right
        1, 2,       // top-left-right, bottom-left-right
        0,          // top-bottom-left-right
    };
    if (edge > 15) {
        return 0;
    }
    return arr[edge];
}

static struct xdg_surface *
get_xdg_surface(struct wlf_surface *surface)
{
    struct wlf_toplevel *t = wlf_toplevel_from_surface(surface);
    if (t) {
        return t->xdg_surface;
    }

    struct wlf_popup *p = wlf_popup_from_surface(surface);
    if (p) {
        return p->xdg_surface;
    }

    return nullptr;
}

static void
wlf_popup_configure(struct wlf_surface *surface, uint32_t serial)
{
    struct wlf_popup *popup = wl_container_of(surface, popup, s);
    enum wlf_surface_event mask = popup->s.event_mask;

    if (mask & WLF_SURFACE_EVENT_REPOSITIONED) {
        popup->current.token = popup->current.token;
    }

    if (mask & WLF_SURFACE_EVENT_OFFSET) {
        popup->current.offset = popup->pending.offset;
    }

    bool resized = false;
    if (mask & WLF_SURFACE_EVENT_EXTENT) {
        popup->s.current.extent = popup->s.pending.extent;
        resized = true;
    }

    bool rescaled = false;
    if (mask & WLF_SURFACE_EVENT_SCALE) {
        surface->current.extent = surface->pending.extent;
        resized = true;
        rescaled = true;
    }

    bool transformed = false;
    if (mask & WLF_SURFACE_EVENT_TRANSFORM) {
        surface->current.transform = surface->pending.transform;
        transformed = true;
        resized |= wlf_transform_is_perpendicular(
            surface->pending.transform,
            surface->current.transform);
    }

    surface->event_mask = WLF_SURFACE_EVENT_NONE;

    if (resized || !surface->configured) {
        struct wlf_extent be = wlf_surface_get_buffer_extent(surface);
        struct wlf_extent se = wlf_surface_get_surface_extent(surface);

        if (surface->wl_egl_window) {
            wl_egl_window_resize(surface->wl_egl_window, be.width, be.height, 0, 0);
        }

        if (surface->wp_fractional_scale_v1) {
            assert(surface->wp_viewport);
            wp_viewport_set_destination(surface->wp_viewport, se.width, se.height);
        }

        popup->listener.configure(popup->s.user_data, be);
    }

    uint32_t version = wl_surface_get_version(surface->wl_surface);
    if (rescaled &&
        !surface->wp_fractional_scale_v1 &&
        version >= WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION)
    {
        wl_surface_set_buffer_scale(surface->wl_surface, surface->current.scale);
    }

    if (transformed && version >= WL_SURFACE_SET_BUFFER_TRANSFORM_SINCE_VERSION) {
        wl_surface_set_buffer_transform(surface->wl_surface, surface->current.transform);
    }
}

// region XDG Popup

static void
xdg_popup_configure(
    void *data,
    struct xdg_popup *xdg_popup,
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height)
{
    struct wlf_popup *popup = data;

    popup->s.event_mask &= ~(WLF_SURFACE_EVENT_EXTENT | WLF_SURFACE_EVENT_OFFSET);

    if (x != popup->current.offset.x || y != popup->current.offset.y) {
        popup->s.event_mask |= WLF_SURFACE_EVENT_OFFSET;
        popup->pending.offset.x = x;
        popup->pending.offset.y = y;
    }

    if (width != popup->s.current.extent.width || height != popup->s.current.extent.height) {
        popup->s.event_mask |= WLF_SURFACE_EVENT_EXTENT;
        popup->s.pending.extent.width = width;
        popup->s.pending.extent.height = height;
    }
}

static void
xdg_popup_popup_done(void *data, struct xdg_popup *xdg_popup)
{
    struct wlf_popup *popup = data;
    popup->listener.done(popup->s.user_data);
}

static void
xdg_popup_repositioned(void *data, struct xdg_popup *xdg_popup, uint32_t token)
{
    struct wlf_popup *popup = data;

    popup->s.event_mask &= ~WLF_SURFACE_EVENT_REPOSITIONED;

    if (token != popup->current.token) {
        popup->s.event_mask |= WLF_SURFACE_EVENT_REPOSITIONED;
        popup->pending.token = token;
    }
}

static const struct xdg_popup_listener xdg_popup_listener = {
    .configure    = xdg_popup_configure,
    .popup_done   = xdg_popup_popup_done,
    .repositioned = xdg_popup_repositioned,
};

// endregion

// region XDG Surface

static void
xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
    struct wlf_popup *popup = data;
    struct wlf_surface *surface = &popup->s;

    if (surface->event_mask != WLF_SURFACE_EVENT_NONE || !surface->configured) {
        surface->configure(surface, serial);
    }

    xdg_surface_ack_configure(popup->xdg_surface, serial);
    surface->configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

// endregion

static void
wlf_popup_init_state(struct wlf_popup *popup, const struct wlf_popup_info *info)
{
    if (popup->s.wp_fractional_scale_v1) {
        popup->s.current.scale = 120;
    } else {
        popup->s.current.scale = 1;
    }
    popup->s.current.transform = WLF_TRANSFORM_NONE;
    popup->s.current.extent.width = 0;
    popup->s.current.extent.height = 0;
    popup->current.offset.x = 0;
    popup->current.offset.y = 0;
    popup->current.token = 0;

    if (info->flags & WLF_POPUP_FLAGS_INHIBIT_IDLING) {
        wlf_surface_inhibit_idling(&popup->s, true);
    }
}

static void
init_positioner(
    struct xdg_positioner *pos,
    const struct wlf_popup_position *p,
    bool reactive)
{
    xdg_positioner_set_size(pos, p->extent.width, p->extent.height);
    xdg_positioner_set_offset(pos, p->offset.x, p->offset.y);
    xdg_positioner_set_anchor(pos, edge_to_xdg(p->anchor_edge));
    xdg_positioner_set_anchor_rect(
        pos,
        p->anchor_rect.offset.x,
        p->anchor_rect.offset.y,
        p->anchor_rect.extent.width,
        p->anchor_rect.extent.height);
    xdg_positioner_set_gravity(pos, edge_to_xdg(p->gravity));
    xdg_positioner_set_constraint_adjustment(pos, p->adjust);
    if (reactive && xdg_positioner_get_version(pos) >= XDG_POSITIONER_SET_REACTIVE_SINCE_VERSION) {
        xdg_positioner_set_reactive(pos);
    }
}

static enum wlf_result
wlf_popup_init(
    struct wlf_context *context,
    const struct wlf_popup_info *info,
    const struct wlf_popup_listener *listener,
    struct wlf_popup *popup)
{
    struct xdg_surface *parent = get_xdg_surface(info->parent);
    if (!parent) {
        return WLF_ERROR_INVALID_ARGUMENT;
    }

    enum wlf_result res = wlf_surface_init(context, WLF_SURFACE_TYPE_POPUP, &popup->s);
    if (res < WLF_SUCCESS) {
        return res;
    }

    popup->listener = *listener;
    popup->s.configure = wlf_popup_configure;
    popup->s.user_data = info->user_data;

    popup->xdg_surface = xdg_wm_base_get_xdg_surface(context->xdg_wm_base, popup->s.wl_surface);
    xdg_surface_add_listener(popup->xdg_surface, &xdg_surface_listener, popup);

    struct xdg_positioner *pos = xdg_wm_base_create_positioner(context->xdg_wm_base);
    init_positioner(pos, &info->position, info->flags & WLF_POPUP_FLAGS_REACTIVE);

    popup->xdg_popup = xdg_surface_get_popup(popup->xdg_surface, parent, pos);
    xdg_popup_add_listener(popup->xdg_popup, &xdg_popup_listener, popup);

    xdg_positioner_destroy(pos);

    wlf_popup_init_state(popup, info);
    wl_surface_commit(popup->s.wl_surface);
    return WLF_SUCCESS;
}

void
wlf_popup_fini(struct wlf_popup *popup)
{
    xdg_popup_destroy(popup->xdg_popup);
    xdg_surface_destroy(popup->xdg_surface);
    wlf_surface_fini(&popup->s);
}

enum wlf_result
wlf_popup_create(
    struct wlf_context *context,
    const struct wlf_popup_info *info,
    const struct wlf_popup_listener *listener,
    struct wlf_popup **_popup)
{
    if (!context->xdg_wm_base) {
        return WLF_ERROR_UNSUPPORTED;
    }

    struct wlf_popup *popup = calloc(1, sizeof(struct wlf_popup));
    if (!popup) {
        return WLF_ERROR_OUT_OF_MEMORY;
    }

    enum wlf_result res = wlf_popup_init(context, info, listener, popup);
    if (res < WLF_SUCCESS) {
        free(popup);
    } else {
        *_popup = popup;
    }
    return res;
}

void
wlf_popup_destroy(struct wlf_popup *popup)
{
    wlf_popup_fini(popup);
    free(popup);
}

struct wlf_surface *
wlf_popup_get_surface(struct wlf_popup *popup)
{
    return &popup->s;
}

struct wlf_popup *
wlf_popup_from_surface(struct wlf_surface *surface)
{
    struct wlf_popup *p = nullptr;
    if (surface->type == WLF_SURFACE_TYPE_POPUP) {
        p = wl_container_of(surface, p, s);
    }
    return p;
}

enum wlf_result
wlf_popup_reposition(
    struct wlf_popup *popup,
    const struct wlf_popup_position *position,
    bool reactive)
{
    return WLF_ERROR_UNKNOWN;
}

enum wlf_result
wlf_popup_grab(struct wlf_popup *popup, struct wlf_seat *seat, uint32_t serial)
{
    if (!seat->wl_seat) {
        return WLF_ERROR_LOST;
    }
    xdg_popup_grab(popup->xdg_popup, seat->wl_seat, serial);
    return WLF_SUCCESS;
}
