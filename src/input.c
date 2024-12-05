#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <locale.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#include <linux/input.h>

#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <input-timestamps-unstable-v1-client-protocol.h>
#include <relative-pointer-unstable-v1-client-protocol.h>
#include <pointer-constraints-unstable-v1-client-protocol.h>
#include <pointer-gestures-unstable-v1-client-protocol.h>
#include <keyboard-shortcuts-inhibit-unstable-v1-client-protocol.h>
#include <text-input-unstable-v3-client-protocol.h>
#include <ext-idle-notify-v1-client-protocol.h>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "common_priv.h"
#include "context_priv.h"
#include "surface_priv.h"
#include "input_priv.h"

extern const uint32_t WLF_WL_SEAT_VERSION;

// region Cursor

static const char *wlf_cursor_names[35] = {
    nullptr,         // WLF_CURSOR_HIDDEN
    "default",       // WLF_CURSOR_DEFAULT
    "context-menu",  // WLF_CURSOR_CONTEXT_MENU
    "help",          // WLF_CURSOR_HELP
    "pointer",       // WLF_CURSOR_POINTER
    "progress",      // WLF_CURSOR_PROGRESS
    "wait",          // WLF_CURSOR_WAIT
    "cell",          // WLF_CURSOR_CELL
    "crosshair",     // WLF_CURSOR_CROSSHAIR
    "text",          // WLF_CURSOR_TEXT
    "vertical-text", // WLF_CURSOR_VERTICAL_TEXT
    "alias",         // WLF_CURSOR_ALIAS
    "copy",          // WLF_CURSOR_COPY
    "move",          // WLF_CURSOR_MOVE
    "no-drop",       // WLF_CURSOR_NO_DROP
    "not-allowed",   // WLF_CURSOR_NOT_ALLOWED
    "grab",          // WLF_CURSOR_GRAB
    "grabbing",      // WLF_CURSOR_GRABBING
    "e-resize",      // WLF_CURSOR_E_RESIZE
    "n-resize",      // WLF_CURSOR_N_RESIZE
    "ne-resize",     // WLF_CURSOR_NE_RESIZE
    "nw-resize",     // WLF_CURSOR_NW_RESIZE
    "s-resize",      // WLF_CURSOR_S_RESIZE
    "se-resize",     // WLF_CURSOR_SE_RESIZE
    "sw-resize",     // WLF_CURSOR_SW_RESIZE
    "w-resize",      // WLF_CURSOR_W_RESIZE
    "ew-resize",     // WLF_CURSOR_EW_RESIZE
    "ns-resize",     // WLF_CURSOR_NS_RESIZE
    "nesw-resize",   // WLF_CURSOR_NESW_RESIZE
    "nwse-resize",   // WLF_CURSOR_NWSE_RESIZE
    "col-resize",    // WLF_CURSOR_COL_RESIZE
    "row-resize",    // WLF_CURSOR_ROW_RESIZE
    "all-scroll",    // WLF_CURSOR_ALL_SCROLL
    "zoom-in",       // WLF_CURSOR_ZOOM_IN
    "zoom-out",      // WLF_CURSOR_ZOOM_OUT
};

static const char *
wlf_cursor_name(enum wlf_cursor cursor)
{
    if (cursor > 34) {
        return nullptr;
    }
    return wlf_cursor_names[cursor];
}

static void
wlf_pointer_set_cursor(struct wlf_pointer *ptr, enum wlf_cursor cursor, uint32_t serial)
{
    const char *name = wlf_cursor_name(cursor);
    if (name) {
        ptr->wl_cursor = wl_cursor_theme_get_cursor(ptr->wl_cursor_theme, name);
        if (ptr->wl_cursor) {
            struct wl_cursor_image *image = ptr->wl_cursor->images[0];
            struct wl_buffer *buffer = wl_cursor_image_get_buffer(image);

            wl_pointer_set_cursor(
                ptr->wl_pointer,
                serial,
                ptr->wl_surface,
                (int32_t) image->hotspot_x,
                (int32_t) image->hotspot_y);

            wl_surface_attach(ptr->wl_surface, buffer, 0, 0);
            wl_surface_commit(ptr->wl_surface);
        }
    } else {
        wl_pointer_set_cursor(ptr->wl_pointer, serial, nullptr, 0, 0);
        wl_surface_attach(ptr->wl_surface, nullptr, 0, 0);
        wl_surface_commit(ptr->wl_surface);
    }
}

// endregion

// region Wp Swipe Gesture

static void
wp_pointer_gesture_swipe_v1_begin(
    void *data,
    struct zwp_pointer_gesture_swipe_v1 *wp_pointer_gesture_swipe_v1,
    uint32_t serial,
    uint32_t time,
    struct wl_surface *surface,
    uint32_t fingers)
{

}

static void
wp_pointer_gesture_swipe_v1_update(
    void *data,
    struct zwp_pointer_gesture_swipe_v1 *zwp_pointer_gesture_swipe_v1,
    uint32_t time,
    wl_fixed_t dx,
    wl_fixed_t dy)
{
}

static void
wp_pointer_gesture_swipe_v1_end(
    void *data,
    struct zwp_pointer_gesture_swipe_v1 *zwp_pointer_gesture_swipe_v1,
    uint32_t serial,
    uint32_t time,
    int32_t cancelled)
{
}

static const struct zwp_pointer_gesture_swipe_v1_listener wp_pointer_gesture_swipe_v1_listenter = {
    .begin  = wp_pointer_gesture_swipe_v1_begin,
    .update = wp_pointer_gesture_swipe_v1_update,
    .end    = wp_pointer_gesture_swipe_v1_end,
};

// endregion

// region Wp Pinch Gesture

static void
wp_pointer_gesture_pinch_v1_begin(
    void *data,
    struct zwp_pointer_gesture_pinch_v1 *wp_pointer_gesture_pinch_v1,
    uint32_t serial,
    uint32_t time,
    struct wl_surface *surface,
    uint32_t fingers)
{
}

static void
wp_pointer_gesture_pinch_v1_update(
    void *data,
    struct zwp_pointer_gesture_pinch_v1 *zwp_pointer_gesture_pinch_v1,
    uint32_t time,
    wl_fixed_t dx,
    wl_fixed_t dy,
    wl_fixed_t scale,
    wl_fixed_t rotation)
{
}

static void
wp_pointer_gesture_pinch_v1_end(
    void *data,
    struct zwp_pointer_gesture_pinch_v1 *zwp_pointer_gesture_pinch_v1,
    uint32_t serial,
    uint32_t time,
    int32_t cancelled)
{
}

static const struct zwp_pointer_gesture_pinch_v1_listener wp_pointer_gesture_pinch_v1_listenter = {
    .begin  = wp_pointer_gesture_pinch_v1_begin,
    .update = wp_pointer_gesture_pinch_v1_update,
    .end    = wp_pointer_gesture_pinch_v1_end,
};

// endregion

// region Wp Hold Gesture

static void
wp_pointer_gesture_hold_v1_begin(
    void *data,
    struct zwp_pointer_gesture_hold_v1 *wp_pointer_gesture_hold_v1,
    uint32_t serial,
    uint32_t time,
    struct wl_surface *surface,
    uint32_t fingers)
{
}

static void
wp_pointer_gesture_hold_v1_end(
    void *data,
    struct zwp_pointer_gesture_hold_v1 *zwp_pointer_gesture_hold_v1,
    uint32_t serial,
    uint32_t time,
    int32_t cancelled)
{
}

static const struct zwp_pointer_gesture_hold_v1_listener wp_pointer_gesture_hold_v1_listenter = {
    .begin  = wp_pointer_gesture_hold_v1_begin,
    .end    = wp_pointer_gesture_hold_v1_end,
};

// endregion

static void
wlf_pointer_constraint_destroy(struct wlf_pointer_constraint *cons)
{
    wl_list_remove(&cons->link);

    if (cons->type == ZWP_POINTER_CONSTRAINTS_V1_LOCK_POINTER) {
        zwp_locked_pointer_v1_destroy(cons->wp_locked_pointer_v1);
    } else if (cons->type == ZWP_POINTER_CONSTRAINTS_V1_CONFINE_POINTER) {
        zwp_confined_pointer_v1_destroy(cons->wp_confined_pointer_v1);
    }

    free(cons);
}

static void
wlf_pointer_frame(struct wlf_pointer *pointer)
{
    if (pointer->frame.mask & WLF_POINTER_FRAME_EVENT_ENTER) {
        wlf_pointer_set_cursor(pointer, WLF_CURSOR_DEFAULT, pointer->frame.serial);
    }

    pointer->frame.mask = WLF_POINTER_FRAME_EVENT_NONE;
}

// region Wp Locked Pointer

static void
wp_pointer_locked(void *data, struct zwp_locked_pointer_v1 *)
{
    struct wlf_pointer_constraint *cons = data;
    struct wlf_pointer *pointer = cons->pointer;

    pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_STATE;
    pointer->frame.state = WLF_POINTER_STATE_LOCKED;

    wlf_pointer_frame(pointer);
}

static void
wp_pointer_unlocked(void *data, struct zwp_locked_pointer_v1 *)
{
    struct wlf_pointer_constraint *cons = data;
    struct wlf_pointer *pointer = cons->pointer;

    pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_STATE;
    pointer->frame.state = WLF_POINTER_STATE_NONE;

    if (cons->lifetime == ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT) {
        wlf_pointer_constraint_destroy(cons);
    }

    wlf_pointer_frame(pointer);
}

static const struct zwp_locked_pointer_v1_listener wp_locked_pointer_v1_listener = {
    .locked = wp_pointer_locked,
    .unlocked = wp_pointer_unlocked,
};

// endregion

// region Wp Confined Pointer

static void
wp_pointer_confined(void *data, struct zwp_confined_pointer_v1 *)
{
    struct wlf_pointer_constraint *cons = data;
    struct wlf_pointer *pointer = cons->pointer;

    pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_STATE;
    pointer->frame.state = WLF_POINTER_STATE_CONFINED;

    wlf_pointer_frame(pointer);
}

static void
wp_pointer_unconfined(void *data, struct zwp_confined_pointer_v1 *)
{
    struct wlf_pointer_constraint *cons = data;
    struct wlf_pointer *pointer = cons->pointer;

    pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_STATE;
    pointer->frame.state = WLF_POINTER_STATE_NONE;

    if (cons->lifetime == ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT) {
        wlf_pointer_constraint_destroy(cons);
    }

    wlf_pointer_frame(pointer);
}

static const struct zwp_confined_pointer_v1_listener wp_confined_pointer_v1_listener = {
    .confined = wp_pointer_confined,
    .unconfined = wp_pointer_unconfined,
};

// endregion

// region Wp Relative Pointer

static void
wp_pointer_relative_motion(
    void *data,
    struct zwp_relative_pointer_v1 *,
    uint32_t utime_hi,
    uint32_t utime_lo,
    wl_fixed_t dx,
    wl_fixed_t dy,
    wl_fixed_t dx_unaccel,
    wl_fixed_t dy_unaccel)
{
    struct wlf_pointer *pointer = data;

    // TODO: The spec doesn't clarify if wp_input_timestamps apply to this event.
    //  Weston doesn't send a timestamp event, so for now just use this value.
    //  There is also a small bug here where if another event follows this one
    //  in the same frame e.g. wl_pointer.motion then the lower resolution timestamp
    //  replaces this one.

    pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_MOTION_RELATIVE;
    pointer->frame.time = wlf_us_to_ns(utime_hi, utime_lo);
    pointer->frame.dx = wl_fixed_to_double(dx);
    pointer->frame.dy = wl_fixed_to_double(dy);
    pointer->frame.udx = wl_fixed_to_double(dx_unaccel);
    pointer->frame.udy = wl_fixed_to_double(dy_unaccel);

    if (wl_pointer_get_version(pointer->wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
        wlf_pointer_frame(pointer);
    }
}

static const struct zwp_relative_pointer_v1_listener wp_relative_pointer_v1_listener = {
    .relative_motion = wp_pointer_relative_motion,
};

// endregion

// region Wp Pointer Timestamp

static void
wp_pointer_timestamp(
    void *data,
    struct zwp_input_timestamps_v1 *,
    uint32_t tv_sec_hi,
    uint32_t tv_sec_lo,
    uint32_t tv_nsec)
{
    struct wlf_pointer *pointer = data;
    pointer->frame.time = wlf_tv_to_ns(tv_sec_hi, tv_sec_lo, tv_nsec);
}

static const struct zwp_input_timestamps_v1_listener wp_pointer_timestamps_v1_listener = {
    .timestamp = wp_pointer_timestamp,
};

// endregion

// region Wl Pointer

static void
wl_pointer_enter(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial,
    struct wl_surface *surface,
    wl_fixed_t sx,
    wl_fixed_t sy)
{
    struct wlf_pointer *pointer = data;

    pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_ENTER;
    pointer->frame.serial = serial;
    pointer->frame.enter = surface ? wl_surface_get_user_data(surface) : nullptr;
    pointer->frame.x = wl_fixed_to_double(sx);
    pointer->frame.y = wl_fixed_to_double(sy);

    if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
        wlf_pointer_frame(pointer);
    }
}

static void
wl_pointer_leave(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial,
    struct wl_surface *surface)
{
    struct wlf_pointer *pointer = data;

    pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_LEAVE;
    pointer->frame.serial = serial;
    pointer->frame.leave = surface ? wl_surface_get_user_data(surface) : nullptr;

    if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
        wlf_pointer_frame(pointer);
    }
}

static void
wl_pointer_motion(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t time,
    wl_fixed_t sx,
    wl_fixed_t sy)
{
    struct wlf_pointer *pointer = data;

    pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_MOTION_ABSOLUTE;
    if (!pointer->wp_timestamps_v1) {
        pointer->frame.time = wlf_ms_to_ns(time);
    }
    pointer->frame.x = wl_fixed_to_double(sx);
    pointer->frame.y = wl_fixed_to_double(sy);

    if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
        wlf_pointer_frame(pointer);
    }
}

static void
wl_pointer_button(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state)
{
    struct wlf_pointer *pointer = data;

    pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_BUTTON;
    pointer->frame.serial = serial;
    if (!pointer->wp_timestamps_v1) {
        pointer->frame.time = wlf_ms_to_ns(time);
    }
    pointer->frame.button.code = button;
    pointer->frame.button.state = state;

    if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
        wlf_pointer_frame(pointer);
    }
}

static void
wl_pointer_axis(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t time,
    uint32_t axis,
    wl_fixed_t value)
{
    struct wlf_pointer *pointer = data;

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
        pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_AXIS_X;
        pointer->frame.axis.x.value = wl_fixed_to_double(value);
    } else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_AXIS_Y;
        pointer->frame.axis.y.value = wl_fixed_to_double(value);
    } else {
        return;
    }

    if (!pointer->wp_timestamps_v1) {
        pointer->frame.time = wlf_ms_to_ns(time);
    }

    if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
        wlf_pointer_frame(pointer);
    }
}

static void
wl_pointer_frame(void *data, struct wl_pointer *)
{
    struct wlf_pointer *pointer = data;

    wlf_pointer_frame(pointer);
}

static void
wl_pointer_axis_source(void *data, struct wl_pointer *, uint32_t axis_source)
{
    struct wlf_pointer *pointer = data;

    pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_AXIS_SOURCE;
    pointer->frame.axis.source = axis_source;
}

static void
wl_pointer_axis_stop(
    void *data,
    struct wl_pointer *,
    uint32_t time,
    uint32_t axis)
{
    struct wlf_pointer *pointer = data;

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
        pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_AXIS_X_STOP;
    } else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_AXIS_Y_STOP;
    } else {
        return;
    }

    if (!pointer->wp_timestamps_v1) {
        pointer->frame.time = wlf_ms_to_ns(time);
    }
}

static void
wl_pointer_axis_discrete(
    void *data,
    struct wl_pointer *,
    uint32_t axis,
    int32_t discrete)
{
    struct wlf_pointer *pointer = data;

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
        pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_AXIS_X_DISCRETE;
        pointer->frame.axis.x.discrete = discrete;
    } else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_AXIS_Y_DISCRETE;
        pointer->frame.axis.y.discrete = discrete;
    }
}

static void
wl_pointer_axis_value120(
    void *data,
    struct wl_pointer *,
    uint32_t axis,
    int32_t value)
{
    struct wlf_pointer *pointer = data;

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
        pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_AXIS_X_DISCRETE;
        pointer->frame.axis.x.discrete = value / 120.0;
    } else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_AXIS_Y_DISCRETE;
        pointer->frame.axis.y.discrete = value / 120.0;
    }
}

#ifdef WL_POINTER_AXIS_RELATIVE_DIRECTION_SINCE_VERSION
static void
wl_pointer_axis_relative_direction(
    void *data,
    struct wl_pointer *,
    uint32_t axis,
    uint32_t direction)
{
    struct wlf_pointer *pointer = data;

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
        pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_AXIS_X_DIRECTION;
        pointer->frame.axis.x.direction = direction;
    } else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        pointer->frame.mask |= WLF_POINTER_FRAME_EVENT_AXIS_Y_DIRECTION;
        pointer->frame.axis.y.direction = direction;
    }
}
#endif

static const struct wl_pointer_listener wl_pointer_listener = {
    .enter = wl_pointer_enter,
    .leave = wl_pointer_leave,
    .motion = wl_pointer_motion,
    .button = wl_pointer_button,
    .axis = wl_pointer_axis,
    .frame = wl_pointer_frame,
    .axis_source = wl_pointer_axis_source,
    .axis_stop = wl_pointer_axis_stop,
    .axis_discrete = wl_pointer_axis_discrete,
    .axis_value120 = wl_pointer_axis_value120,
#ifdef WL_POINTER_AXIS_RELATIVE_DIRECTION_SINCE_VERSION
    .axis_relative_direction = wl_pointer_axis_relative_direction,
#endif
};

// endregion

static struct wlf_pointer_constraint *
wlf_pointer_find_constraint(struct wlf_pointer *pointer, struct wlf_surface *surface)
{
    if (!pointer->wl_pointer) {
        return nullptr;
    }

    struct wlf_pointer_constraint *cons;
    wl_list_for_each(cons, &pointer->constraints, link) {
        if (cons->surface == surface) {
            return cons;
        }
    }

    return nullptr;
}

static void
wlf_seat_init_pointer(struct wlf_seat *seat)
{
    assert(!seat->pointer.wl_pointer);

    struct wlf_context *ctx = seat->global.context;
    wl_list_init(&seat->pointer.constraints);

    const char *theme = getenv("XCURSOR_THEME");
    if (!theme) {
        theme = "default";
    }
    const char *size_str = getenv("XCURSOR_SIZE");
    int size = 24;
    if (size_str) {
        long s = strtol(size_str, nullptr, 10);
        if (s > 0 && s != LONG_MAX) {
            size = (int)s;
        }
    }

    seat->pointer.wl_pointer = wl_seat_get_pointer(seat->wl_seat);
    wl_pointer_add_listener(seat->pointer.wl_pointer, &wl_pointer_listener, &seat->pointer);

    seat->pointer.wl_cursor_theme = wl_cursor_theme_load(theme, size, ctx->wl_shm);
    seat->pointer.wl_surface = wl_compositor_create_surface(ctx->wl_compositor);

    if (ctx->wp_input_timestamps_manager_v1) {
        seat->pointer.wp_timestamps_v1 = zwp_input_timestamps_manager_v1_get_pointer_timestamps(
            ctx->wp_input_timestamps_manager_v1,
            seat->pointer.wl_pointer);
        zwp_input_timestamps_v1_add_listener(
            seat->pointer.wp_timestamps_v1,
            &wp_pointer_timestamps_v1_listener,
            &seat->pointer);
    }

    if (ctx->wp_relative_pointer_manager_v1) {
        seat->pointer.wp_relative_v1 = zwp_relative_pointer_manager_v1_get_relative_pointer(
            ctx->wp_relative_pointer_manager_v1,
            seat->pointer.wl_pointer);
        zwp_relative_pointer_v1_add_listener(
            seat->pointer.wp_relative_v1,
            &wp_relative_pointer_v1_listener,
            &seat->pointer);
    }

    if (ctx->wp_pointer_gestures_v1) {
        seat->pointer.wp_swipe_v1 = zwp_pointer_gestures_v1_get_swipe_gesture(
            ctx->wp_pointer_gestures_v1,
            seat->pointer.wl_pointer);
        zwp_pointer_gesture_swipe_v1_add_listener(
            seat->pointer.wp_swipe_v1,
            &wp_pointer_gesture_swipe_v1_listenter,
            &seat->pointer);

        seat->pointer.wp_pinch_v1 = zwp_pointer_gestures_v1_get_pinch_gesture(
            ctx->wp_pointer_gestures_v1,
            seat->pointer.wl_pointer);
        zwp_pointer_gesture_pinch_v1_add_listener(
            seat->pointer.wp_pinch_v1,
            &wp_pointer_gesture_pinch_v1_listenter,
            &seat->pointer);

        uint32_t version = zwp_pointer_gestures_v1_get_version(ctx->wp_pointer_gestures_v1);

        if (version >= ZWP_POINTER_GESTURES_V1_GET_HOLD_GESTURE_SINCE_VERSION) {
            seat->pointer.wp_hold_v1 = zwp_pointer_gestures_v1_get_hold_gesture(
                ctx->wp_pointer_gestures_v1,
                seat->pointer.wl_pointer);
            zwp_pointer_gesture_hold_v1_add_listener(
                seat->pointer.wp_hold_v1,
                &wp_pointer_gesture_hold_v1_listenter,
                &seat->pointer);
        }
    }
}

static void
wlf_seat_fini_pointer(struct wlf_seat *seat)
{
    struct wlf_pointer *pointer = &seat->pointer;
    assert(pointer->wl_pointer);

    struct wlf_pointer_constraint *cons, *tmp;
    wl_list_for_each_safe(cons, tmp, &pointer->constraints, link) {
        wlf_pointer_constraint_destroy(cons);
    }

    if (pointer->wp_swipe_v1) {
        zwp_pointer_gesture_swipe_v1_destroy(pointer->wp_swipe_v1);
    }

    if (pointer->wp_pinch_v1) {
        zwp_pointer_gesture_pinch_v1_destroy(pointer->wp_pinch_v1);
    }

    if (pointer->wp_hold_v1) {
        zwp_pointer_gesture_hold_v1_destroy(pointer->wp_hold_v1);
    }

    if (pointer->wp_timestamps_v1) {
        zwp_input_timestamps_v1_destroy(pointer->wp_timestamps_v1);
    }

    if (pointer->wp_relative_v1) {
        zwp_relative_pointer_v1_destroy(pointer->wp_relative_v1);
    }

    if (wl_pointer_get_version(pointer->wl_pointer) >= WL_POINTER_RELEASE_SINCE_VERSION) {
        wl_pointer_release(pointer->wl_pointer);
    } else {
        wl_pointer_destroy(pointer->wl_pointer);
    }

    wl_cursor_theme_destroy(pointer->wl_cursor_theme);
    wl_surface_destroy(pointer->wl_surface);

    memset(pointer, 0, sizeof(struct wlf_pointer));
}

// region Wp Keyboard Timestamp

static void
wp_keyboard_timestamp(
    void *data,
    struct zwp_input_timestamps_v1 *wp_input_timestamps_v1,
    uint32_t tv_sec_hi,
    uint32_t tv_sec_lo,
    uint32_t tv_nsec)
{
    struct wlf_keyboard *keyboard = data;
    keyboard->event_time = wlf_tv_to_ns(tv_sec_hi, tv_sec_lo, tv_nsec);
}

static const struct zwp_input_timestamps_v1_listener wp_keyboard_timestamps_v1_listener = {
    .timestamp = wp_keyboard_timestamp,
};

// endregion

// region Wl Keyboard

static void
wl_keyboard_keymap(
    void *data,
    struct wl_keyboard *,
    uint32_t format,
    int32_t fd,
    uint32_t size)
{
    struct wlf_keyboard *keyboard = data;

    xkb_compose_state_unref(keyboard->xkb_compose_state);
    keyboard->xkb_compose_state = nullptr;

    xkb_state_unref(keyboard->xkb_state);
    keyboard->xkb_state = nullptr;

    xkb_keymap_unref(keyboard->xkb_keymap);
    keyboard->xkb_keymap = nullptr;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    char *map_str = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (map_str == MAP_FAILED) {
        return;
    }

    struct xkb_keymap *keymap = xkb_keymap_new_from_string(
        keyboard->xkb_context,
        map_str,
        XKB_KEYMAP_FORMAT_TEXT_V1,
        XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_str, size);
    if (!keymap) {
        return;
    }

    struct xkb_state *state = xkb_state_new(keymap);
    if (!state) {
        xkb_keymap_unref(keymap);
        return;
    }

    struct xkb_compose_state *compose_state = nullptr;
    if (keyboard->locale) {
        struct xkb_compose_table *table = xkb_compose_table_new_from_locale(
            keyboard->xkb_context,
            keyboard->locale,
            XKB_COMPOSE_COMPILE_NO_FLAGS);
        if (table) {
            compose_state = xkb_compose_state_new(table, XKB_COMPOSE_STATE_NO_FLAGS);
            xkb_compose_table_unref(table);
        }
    }
    keyboard->xkb_compose_state = compose_state;
    keyboard->xkb_keymap = keymap;
    keyboard->xkb_state = state;
}

static void
wl_keyboard_enter(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32_t serial,
    struct wl_surface *wl_surface,
    struct wl_array *keys)
{
}

static void
wl_keyboard_leave(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32_t serial,
    struct wl_surface *wl_surface)
{
}

static void
wl_keyboard_key(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32_t serial,
    uint32_t time,
    uint32_t key,
    uint32_t state)
{
}

static void
wl_keyboard_modifiers(
    void *data,
    struct wl_keyboard *,
    uint32_t,
    uint32_t mods_depressed,
    uint32_t mods_latched,
    uint32_t mods_locked,
    uint32_t group)
{
    struct wlf_keyboard *keyboard = data;

    if (keyboard->xkb_state) {
        xkb_state_update_mask(
            keyboard->xkb_state,
            mods_depressed,
            mods_latched,
            mods_locked,
            0,
            0,
            group);
    }
}

static void
wl_keyboard_repeat_info(
    void *data,
    struct wl_keyboard *,
    int32_t rate,
    int32_t delay)
{
    struct wlf_keyboard *keyboard = data;

    keyboard->repeat_rate = rate;
    keyboard->repeat_delay = delay;
}

static const struct wl_keyboard_listener wl_keyboard_listener = {
    .keymap      = wl_keyboard_keymap,
    .enter       = wl_keyboard_enter,
    .leave       = wl_keyboard_leave,
    .key         = wl_keyboard_key,
    .modifiers   = wl_keyboard_modifiers,
    .repeat_info = wl_keyboard_repeat_info,
};

// endregion

static void
wlf_seat_init_keyboard(struct wlf_seat *seat)
{
    assert(!seat->keyboard.wl_keyboard);

    struct wlf_context *ctx = seat->global.context;

    seat->keyboard.xkb_context = xkb_context_ref(ctx->xkb_context);

    seat->keyboard.wl_keyboard = wl_seat_get_keyboard(seat->wl_seat);
    wl_keyboard_add_listener(seat->keyboard.wl_keyboard, &wl_keyboard_listener, &seat->keyboard);

    if (ctx->wp_input_timestamps_manager_v1) {
        seat->keyboard.wp_timestamps_v1 = zwp_input_timestamps_manager_v1_get_keyboard_timestamps(
            ctx->wp_input_timestamps_manager_v1,
            seat->keyboard.wl_keyboard);
        zwp_input_timestamps_v1_add_listener(
            seat->keyboard.wp_timestamps_v1,
            &wp_keyboard_timestamps_v1_listener,
            &seat->keyboard);
    }

    seat->keyboard.locale = setlocale(LC_CTYPE_MASK, nullptr);

    seat->keyboard.event_time = -1;
}

static void
wlf_seat_fini_keyboard(struct wlf_seat *seat)
{
    struct wlf_keyboard *keyboard = &seat->keyboard;
    assert(keyboard->wl_keyboard);

    if (keyboard->wp_timestamps_v1) {
        zwp_input_timestamps_v1_destroy(keyboard->wp_timestamps_v1);
    }

    if (wl_keyboard_get_version(keyboard->wl_keyboard) >= WL_KEYBOARD_RELEASE_SINCE_VERSION) {
        wl_keyboard_release(keyboard->wl_keyboard);
    } else {
        wl_keyboard_destroy(keyboard->wl_keyboard);
    }

    xkb_compose_state_unref(keyboard->xkb_compose_state);
    xkb_state_unref(keyboard->xkb_state);
    xkb_keymap_unref(keyboard->xkb_keymap);
    xkb_context_unref(keyboard->xkb_context);

    memset(keyboard, 0, sizeof(struct wlf_keyboard));
}

// region Wp Touch Timestamps

static void
wp_touch_timestamp(
    void *data,
    struct zwp_input_timestamps_v1 *,
    uint32_t tv_sec_hi,
    uint32_t tv_sec_lo,
    uint32_t tv_nsec)
{
}

static const struct zwp_input_timestamps_v1_listener wp_touch_timestamps_v1_listener = {
    .timestamp = wp_touch_timestamp,
};

// endregion

// region Wl Touch

static void
wl_touch_down(
    void *data,
    struct wl_touch *wl_touch,
    uint32_t serial,
    uint32_t time,
    struct wl_surface *wl_surface,
    int32_t id,
    wl_fixed_t x,
    wl_fixed_t y)
{
}

static void
wl_touch_up(
    void *data,
    struct wl_touch *wl_touch,
    uint32_t serial,
    uint32_t time,
    int32_t id)
{
}

static void
wl_touch_motion(
    void *data,
    struct wl_touch *wl_touch,
    uint32_t time,
    int32_t id,
    wl_fixed_t x,
    wl_fixed_t y)
{
}

static void
wl_touch_frame(void *data, struct wl_touch *)
{
}

static void
wl_touch_cancel(void *data, struct wl_touch *wl_touch)
{

}

static void
wl_touch_shape(
    void *data,
    struct wl_touch *wl_touch,
    int32_t id,
    wl_fixed_t major,
    wl_fixed_t minor)
{

}

static void
wl_touch_orientation(
    void *data,
    struct wl_touch *wl_touch,
    int32_t id,
    wl_fixed_t orientation)
{

}

struct wl_touch_listener wl_touch_listener = {
    .down        = wl_touch_down,
    .up          = wl_touch_up,
    .motion      = wl_touch_motion,
    .frame       = wl_touch_frame,
    .cancel      = wl_touch_cancel,
    .shape       = wl_touch_shape,
    .orientation = wl_touch_orientation,
};

// endregion

[[maybe_unused]]
static void
wlf_seat_init_touch(struct wlf_seat *seat)
{
    assert(seat->touch.wl_touch == nullptr);
    assert(seat->touch.wp_timestamps_v1 == nullptr);

    struct wlf_context *ctx = seat->global.context;

    seat->touch.wl_touch = wl_seat_get_touch(seat->wl_seat);
    wl_touch_add_listener(seat->touch.wl_touch, &wl_touch_listener, &seat->touch);

    if (ctx->wp_input_timestamps_manager_v1) {
        seat->touch.wp_timestamps_v1 = zwp_input_timestamps_manager_v1_get_touch_timestamps(
            ctx->wp_input_timestamps_manager_v1,
            seat->touch.wl_touch);
        zwp_input_timestamps_v1_add_listener(
            seat->touch.wp_timestamps_v1,
            &wp_touch_timestamps_v1_listener,
            &seat->touch);
    }
}

[[maybe_unused]]
static void
wlf_seat_fini_touch(struct wlf_seat *seat)
{
    if (seat->touch.wp_timestamps_v1) {
        zwp_input_timestamps_v1_destroy(seat->touch.wp_timestamps_v1);
    }

    if (wl_touch_get_version(seat->touch.wl_touch) >= WL_TOUCH_RELEASE_SINCE_VERSION) {
        wl_touch_release(seat->touch.wl_touch);
    } else {
        wl_touch_destroy(seat->touch.wl_touch);
    }

    memset(&seat->touch, 0, sizeof(struct wlf_touch));
}

// region WL Data Offer

static void
wl_data_offer_offer(
    void *data,
    struct wl_data_offer *wl_data_offer,
    const char *mime_type)
{
}

static void
wl_data_offer_source_actions(
    void *data,
    struct wl_data_offer *wl_data_offer,
    uint32_t source_action)
{
}

static void
wl_data_offer_action(
    void *data,
    struct wl_data_offer *wl_data_offer,
    uint32_t dnd_action)
{
}

[[maybe_unused]]
static const struct wl_data_offer_listener wl_data_offer_listener = {
    .offer          = wl_data_offer_offer,
    .source_actions = wl_data_offer_source_actions,
    .action         = wl_data_offer_action,
};

// endregion

// region Wl Data Device

static void
wl_data_device_data_offer(
    void *data,
    struct wl_data_device *wl_data_device,
    struct wl_data_offer *id)
{
}

static void
wl_data_device_enter(
    void *data,
    struct wl_data_device *wl_data_device,
    uint32_t serial,
    struct wl_surface *wl_surface,
    wl_fixed_t x,
    wl_fixed_t y,
    struct wl_data_offer *id)
{
}

static void
wl_data_device_leave(
    void *data,
    struct wl_data_device *wl_data_device)
{
}

static void
wl_data_device_motion(
    void *data,
    struct wl_data_device *wl_data_device,
    uint32_t time,
    wl_fixed_t x,
    wl_fixed_t y)
{
}

static void
wl_data_device_drop(
    void *data,
    struct wl_data_device *wl_data_device)
{
}

static void
wl_data_device_selection(
    void *data,
    struct wl_data_device *wl_data_device,
    struct wl_data_offer *id)
{
}

static const struct wl_data_device_listener wl_data_device_listener = {
    .data_offer = wl_data_device_data_offer,
    .enter      = wl_data_device_enter,
    .leave      = wl_data_device_leave,
    .motion     = wl_data_device_motion,
    .drop       = wl_data_device_drop,
    .selection  = wl_data_device_selection,
};

// endregion

[[maybe_unused]]
static void
wlf_seat_init_clipboard(struct wlf_seat *seat)
{
    struct wlf_context *ctx = seat->global.context;

    assert(ctx->wl_data_device_manager != nullptr);
    assert(seat->clipboard.wl_data_device == nullptr);

    seat->clipboard.wl_data_device = wl_data_device_manager_get_data_device(
        ctx->wl_data_device_manager,
        seat->wl_seat);
    wl_data_device_add_listener(
        seat->clipboard.wl_data_device,
        &wl_data_device_listener,
        &seat->clipboard);
}

[[maybe_unused]]
static void
wlf_seat_fini_clipboard(struct wlf_seat *seat)
{
    assert(seat->clipboard.wl_data_device != nullptr);

    if (wl_data_device_get_version(seat->clipboard.wl_data_device)
        >= WL_DATA_DEVICE_RELEASE_SINCE_VERSION)
    {
        wl_data_device_release(seat->clipboard.wl_data_device);
    } else {
        wl_data_device_destroy(seat->clipboard.wl_data_device);
    }
    seat->clipboard.wl_data_device = nullptr;
}

// region WP Text Input

static void
wp_text_input_v3_enter(
    void *data,
    struct zwp_text_input_v3 *,
    struct wl_surface *surface)
{
}

static void
wp_text_input_v3_leave(
    void *data,
    struct zwp_text_input_v3 *,
    struct wl_surface *surface)
{
}

static void
wp_text_input_v3_preedit_string(
    void *data,
    struct zwp_text_input_v3 *,
    const char *text,
    int32_t cursor_begin,
    int32_t cursor_end)
{
}

static void
wp_text_input_v3_commit_string(
    void *data,
    struct zwp_text_input_v3 *,
    const char *text)
{
}

static void
wp_text_input_v3_delete_surrounding_text(
    void *data,
    struct zwp_text_input_v3 *,
    uint32_t cursor_begin,
    uint32_t cursor_end)
{
}

static void
wp_text_input_v3_done(
    void *data,
    struct zwp_text_input_v3 *,
    uint32_t cursor_begin)
{
}

static const struct zwp_text_input_v3_listener wp_text_input_v3_listener = {
    .enter                   = wp_text_input_v3_enter,
    .leave                   = wp_text_input_v3_leave,
    .preedit_string          = wp_text_input_v3_preedit_string,
    .commit_string           = wp_text_input_v3_commit_string,
    .delete_surrounding_text = wp_text_input_v3_delete_surrounding_text,
    .done                    = wp_text_input_v3_done,
};

// endregion

[[maybe_unused]]
static void
wlf_seat_init_text_input(struct wlf_seat *seat)
{
    struct wlf_context *ctx = seat->global.context;

    assert(ctx->wp_text_input_manager_v3 != nullptr);
    assert(seat->text_input.wp_text_input_v3 == nullptr);

    seat->text_input.wp_text_input_v3 = zwp_text_input_manager_v3_get_text_input(
        ctx->wp_text_input_manager_v3,
        seat->wl_seat);
    zwp_text_input_v3_add_listener(
        seat->text_input.wp_text_input_v3,
        &wp_text_input_v3_listener,
        &seat->text_input);
}

[[maybe_unused]]
static void
wlf_seat_fini_text_input(struct wlf_seat *seat)
{
    assert(seat->text_input.wp_text_input_v3 != nullptr);

    zwp_text_input_v3_destroy(seat->text_input.wp_text_input_v3);
    seat->text_input.wp_text_input_v3 = nullptr;
}

// region Wp Keyboard Shortcuts Inhibitor

static void
wp_keyboard_shortcuts_inhibitor_v1_active(
    void *data,
    struct zwp_keyboard_shortcuts_inhibitor_v1 *)
{
    struct wlf_shortcuts_inhibitor *si = data;
    struct wlf_seat *seat = si->seat;
    seat->listener.shortcuts_inhibited(seat->user_data, si->surface, true);
}

static void
wp_keyboard_shortcuts_inhibitor_v1_inactive(
    void *data,
    struct zwp_keyboard_shortcuts_inhibitor_v1 *)
{
    struct wlf_shortcuts_inhibitor *si = data;
    struct wlf_seat *seat = si->seat;
    seat->listener.shortcuts_inhibited(seat->user_data, si->surface, false);
}

static const struct zwp_keyboard_shortcuts_inhibitor_v1_listener wp_keyboard_shortcuts_inhibitor_v1_listener = {
    .active = wp_keyboard_shortcuts_inhibitor_v1_active,
    .inactive = wp_keyboard_shortcuts_inhibitor_v1_inactive,
};

// endregion

static struct wlf_shortcuts_inhibitor *
wlf_seat_find_shortcuts_inhibitor(struct wlf_seat *seat, struct wlf_surface *surface)
{
    if (!seat->wl_seat) {
        return nullptr;
    }

    struct wlf_shortcuts_inhibitor *inhibitor;
    wl_list_for_each(inhibitor, &seat->shortcut_inhibitors, link) {
        if (inhibitor->surface == surface) {
            return inhibitor;
        }
    }

    return nullptr;
}

static void
wlf_shortcut_inhibitor_destroy(struct wlf_shortcuts_inhibitor *inhibitor)
{
    assert(inhibitor->wp_shortcuts_inhibitor_v1);
    wl_list_remove(&inhibitor->link);
    zwp_keyboard_shortcuts_inhibitor_v1_destroy(inhibitor->wp_shortcuts_inhibitor_v1);
    free(inhibitor);
}

// region Ext Idle Notification

static void
ext_idle_notification_v1_idled(void *data, struct ext_idle_notification_v1 *)
{
    struct wlf_seat *seat = data;
    seat->listener.idled(seat->user_data, true);
}

static void
ext_idle_notification_v1_resumed(void *data, struct ext_idle_notification_v1 *)
{
    struct wlf_seat *seat = data;
    seat->listener.idled(seat->user_data, false);
}

static const struct ext_idle_notification_v1_listener ext_idle_notification_v1_listener = {
    .idled = ext_idle_notification_v1_idled,
    .resumed = ext_idle_notification_v1_resumed,
};

// endregion

// region WL Seat

static void
wl_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities)
{
    struct wlf_seat *seat = data;

    bool has_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
    bool has_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

    if (has_pointer && !seat->pointer.wl_pointer) {
        wlf_seat_init_pointer(seat);
    } else if (!has_pointer && seat->pointer.wl_pointer) {
        wlf_seat_fini_pointer(seat);
    }

    if (has_keyboard && !seat->keyboard.wl_keyboard) {
        wlf_seat_init_keyboard(seat);
    } else if (!has_keyboard && seat->keyboard.wl_keyboard) {
        wlf_seat_fini_keyboard(seat);
    }
}

static void
wl_seat_name(void *data, struct wl_seat *wl_seat, const char *name)
{
    struct wlf_seat *seat = data;
    if (seat->listener.name) {
        seat->listener.name(seat->user_data, (const char8_t *)name);
    }
}

static const struct wl_seat_listener wl_seat_listener = {
    .capabilities = wl_seat_capabilities,
    .name         = wl_seat_name,
};

// endregion

static enum wlf_result
wlf_seat_init(struct wlf_seat *seat, const struct wlf_seat_listener *listener, const struct wlf_seat_info *info)
{
    assert(!seat->wl_seat);
    assert(seat->global.name != 0);

    struct wlf_context *context = seat->global.context;
    seat->listener = *listener;
    seat->user_data = info->user_data;
    wl_list_init(&seat->shortcut_inhibitors);

    uint32_t version = wlf_get_version(
        &wl_seat_interface,
        seat->global.version,
        WLF_WL_SEAT_VERSION);

    seat->wl_seat = wl_registry_bind(
        context->wl_registry,
        seat->global.name,
        &wl_seat_interface,
        version);
    if (!seat->wl_seat) {
        return WLF_ERROR_WAYLAND;
    }

    wl_seat_add_listener(seat->wl_seat, &wl_seat_listener, seat);
    return WLF_SUCCESS;
}

static void
wlf_seat_fini(struct wlf_seat *seat)
{
    assert(seat->wl_seat);

    if (seat->pointer.wl_pointer) {
        wlf_seat_fini_pointer(seat);
    }

    if (seat->keyboard.wl_keyboard) {
        wlf_seat_fini_keyboard(seat);
    }

    if (seat->touch.wl_touch) {
        wlf_seat_fini_touch(seat);
    }

    if (seat->clipboard.wl_data_device) {
        wlf_seat_fini_clipboard(seat);
    }

    if (seat->text_input.wp_text_input_v3) {
        wlf_seat_fini_text_input(seat);
    }

    struct wlf_shortcuts_inhibitor *inhibitor, *tmp;
    wl_list_for_each_safe(inhibitor, tmp, &seat->shortcut_inhibitors, link) {
        wlf_shortcut_inhibitor_destroy(inhibitor);
    }

    if (seat->ext_idle_notification_v1) {
        ext_idle_notification_v1_destroy(seat->ext_idle_notification_v1);
        seat->ext_idle_notification_v1 = nullptr;
    }

    if (wl_seat_get_version(seat->wl_seat) >= WL_SEAT_RELEASE_SINCE_VERSION) {
        wl_seat_release(seat->wl_seat);
    } else {
        wl_seat_destroy(seat->wl_seat);
    }
    seat->wl_seat = nullptr;
}

void
wlf_seat_handle_surface_destroyed(struct wlf_seat *seat, struct wlf_surface *surface)
{
    // The seat isn't initialized, so there is nothing to do.
    // This can only happen if the global was removed, but a
    // pointer to the seat still exits, held by the calling
    // program.
    if (!seat->wl_seat) {
        return;
    }

    struct wlf_shortcuts_inhibitor *inhibitor
        = wlf_seat_find_shortcuts_inhibitor(seat, surface);
    if (inhibitor) {
        wlf_shortcut_inhibitor_destroy(inhibitor);
    }

    struct wlf_pointer_constraint *constraint
        = wlf_pointer_find_constraint(&seat->pointer, surface);
    if (constraint) {
        wlf_pointer_constraint_destroy(constraint);
    }
}

struct wlf_seat *
wlf_seat_add(struct wlf_context *context, uint32_t name, uint32_t version)
{
    struct wlf_seat *seat = calloc(1, sizeof(struct wlf_seat));
    if (!seat) {
        return nullptr;
    }

    seat->global.context = context;
    seat->global.id = wlf_new_id(context);
    seat->global.name = name;
    seat->global.version = version;

    wl_list_insert(&context->seat_list, &seat->link);
    return seat;
}

void
wlf_seat_remove(struct wlf_seat *seat)
{
    assert(seat->global.name != 0);

    wl_list_remove(&seat->link);
    seat->global.name = 0;
    seat->global.version = 0;

    if (seat->wl_seat) {
        wlf_seat_fini(seat);
        if (seat->listener.lost) {
            seat->listener.lost(seat->user_data);
        }
    } else {
        free(seat);
    }
}

void
wlf_destroy_seats(struct wlf_context *context)
{
    struct wlf_seat *seat, *tmp;
    wl_list_for_each_safe(seat, tmp, &context->seat_list, link) {
        wl_list_remove(&seat->link);
        if (seat->wl_seat) {
            wlf_seat_fini(seat);
        }
        free(seat);
    }
}

static struct wlf_seat *
wlf_seat_find(struct wlf_context *context, uint64_t id)
{
    struct wlf_seat *seat;
    wl_list_for_each(seat, &context->seat_list, link) {
        if (seat->global.id == id) {
            return seat;
        }
    }
    return nullptr;
}

// region Public API

enum wlf_result
wlf_seat_bind(
    struct wlf_context *context,
    const struct wlf_seat_info *info,
    const struct wlf_seat_listener *listener,
    struct wlf_seat **_seat)
{
    struct wlf_seat *seat = wlf_seat_find(context, info->id);
    if (!seat) {
        return WLF_ERROR_LOST;
    }

    if (seat->wl_seat) {
        *_seat = seat;
        return WLF_SKIPPED;
    }

    enum wlf_result res = wlf_seat_init(seat, listener, info);
    if (res >= WLF_SUCCESS) {
        *_seat = seat;
    }
    return res;
}

void
wlf_seat_release(struct wlf_seat *seat)
{
    if (seat->wl_seat) {
        wlf_seat_fini(seat);
    } else {
        free(seat);
    }
}

enum wlf_result
wlf_seat_inhibit_shortcuts(struct wlf_seat *seat, struct wlf_surface *surface, bool inhibit)
{
    if (!seat->wl_seat) {
        return WLF_ERROR_LOST;
    }

    struct wlf_shortcuts_inhibitor *inhibitor = wlf_seat_find_shortcuts_inhibitor(seat, surface);
    if (!inhibit) {
        if (!inhibitor) {
            return WLF_SKIPPED;
        }
        wlf_shortcut_inhibitor_destroy(inhibitor);
        return WLF_SUCCESS;
    }

    if (inhibitor) {
        return WLF_ALREADY_SET;
    }

    struct wlf_context *ctx = seat->global.context;
    if (!ctx->wp_keyboard_shortcuts_inhibit_manager_v1) {
        return WLF_ERROR_UNSUPPORTED;
    }

    inhibitor = calloc(1, sizeof(struct wlf_shortcuts_inhibitor));
    if (!inhibitor) {
        return WLF_ERROR_OUT_OF_MEMORY;
    }

    inhibitor->seat = seat;
    inhibitor->surface = surface;
    inhibitor->wp_shortcuts_inhibitor_v1 = zwp_keyboard_shortcuts_inhibit_manager_v1_inhibit_shortcuts(
        ctx->wp_keyboard_shortcuts_inhibit_manager_v1,
        surface->wl_surface,
        seat->wl_seat);
    if (!inhibitor->wp_shortcuts_inhibitor_v1) {
        free(inhibitor);
        return WLF_ERROR_WAYLAND;
    }

    zwp_keyboard_shortcuts_inhibitor_v1_add_listener(
        inhibitor->wp_shortcuts_inhibitor_v1,
        &wp_keyboard_shortcuts_inhibitor_v1_listener,
        inhibitor);

    wl_list_insert(&seat->shortcut_inhibitors, &inhibitor->link);
    return WLF_SUCCESS;
}

enum wlf_result
wlf_seat_set_idle_time(struct wlf_seat *seat, int64_t time)
{
    if (!seat->wl_seat) {
        return WLF_ERROR_LOST;
    }

    if (time < 0) {
        if (!seat->ext_idle_notification_v1) {
            return WLF_SKIPPED;
        }
        ext_idle_notification_v1_destroy(seat->ext_idle_notification_v1);
        seat->ext_idle_notification_v1 = nullptr;
        return WLF_SUCCESS;
    }

    struct wlf_context *context = seat->global.context;
    if (!context->ext_idle_notifier_v1) {
        return WLF_ERROR_UNSUPPORTED;
    }

    time /= 1'000'000;
    uint32_t time_ms = time > UINT32_MAX ? UINT32_MAX : (uint32_t)time;

    seat->ext_idle_notification_v1 = ext_idle_notifier_v1_get_idle_notification(
        context->ext_idle_notifier_v1,
        time_ms,
        seat->wl_seat);
    if (!seat->ext_idle_notification_v1) {
        return WLF_ERROR_WAYLAND;
    }
    ext_idle_notification_v1_add_listener(
        seat->ext_idle_notification_v1,
        &ext_idle_notification_v1_listener,
        seat);

    return WLF_SUCCESS;
}

// endregion

// region Pointer

enum wlf_result
wlf_pointer_constrain(
    struct wlf_pointer *pointer,
    uint32_t type,
    struct wlf_surface *surface,
    bool persistent,
    double pos_hint[2])
{
    struct wlf_seat *seat = wl_container_of(pointer, seat, pointer);
    if (!seat->wl_seat) {
        return WLF_ERROR_LOST;
    }

    struct wlf_context *context = seat->global.context;

    assert(context->wp_pointer_constraints_v1);
    assert(pointer->wl_pointer);
    assert(type == ZWP_POINTER_CONSTRAINTS_V1_LOCK_POINTER ||
           type == ZWP_POINTER_CONSTRAINTS_V1_CONFINE_POINTER);

    struct wlf_pointer_constraint *cons = wlf_pointer_find_constraint(pointer, surface);
    if (cons) {
        return WLF_ALREADY_SET;
    }

    cons = calloc(1, sizeof(struct wlf_pointer_constraint));
    if (!cons) {
        return WLF_ERROR_OUT_OF_MEMORY;
    }

    cons->pointer = pointer;
    cons->surface = surface;
    cons->type = type;
    cons->lifetime = persistent
                   ? ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT
                   : ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT;

    if (type == ZWP_POINTER_CONSTRAINTS_V1_LOCK_POINTER) {
        cons->wp_locked_pointer_v1 = zwp_pointer_constraints_v1_lock_pointer(
            context->wp_pointer_constraints_v1,
            surface->wl_surface,
            pointer->wl_pointer,
            nullptr,
            cons->lifetime);
        if (!cons->wp_locked_pointer_v1) {
            free(cons);
            return WLF_ERROR_WAYLAND;
        }

        zwp_locked_pointer_v1_add_listener(
            cons->wp_locked_pointer_v1,
            &wp_locked_pointer_v1_listener,
            cons);

        if (pos_hint) {
            zwp_locked_pointer_v1_set_cursor_position_hint(
                cons->wp_locked_pointer_v1,
                wl_fixed_from_double(pos_hint[0]),
                wl_fixed_from_double(pos_hint[1]));
        }
    } else {
        cons->wp_confined_pointer_v1 = zwp_pointer_constraints_v1_confine_pointer(
            context->wp_pointer_constraints_v1,
            surface->wl_surface,
            pointer->wl_pointer,
            nullptr,
            cons->lifetime);
        if (!cons->wp_confined_pointer_v1) {
            free(cons);
            return WLF_ERROR_WAYLAND;
        }

        zwp_confined_pointer_v1_add_listener(
            cons->wp_confined_pointer_v1,
            &wp_confined_pointer_v1_listener,
            cons);
    }

    wl_list_insert(&pointer->constraints, &cons->link);
    return WLF_SUCCESS;
}

enum wlf_result
wlf_pointer_remove_constraint(struct wlf_pointer *pointer, struct wlf_surface *surface)
{
    struct wlf_seat *seat = wl_container_of(pointer, seat, pointer);
    if (!seat->wl_seat) {
        return WLF_ERROR_LOST;
    }
    struct wlf_pointer_constraint *cons = wlf_pointer_find_constraint(pointer, surface);
    if (!cons) {
        return WLF_SKIPPED;
    }
    wlf_pointer_constraint_destroy(cons);
    return WLF_SUCCESS;
}

// endregion

