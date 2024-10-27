#pragma once

#include "wlf/input.h"

enum wlf_pointer_frame_event : uint32_t {
    WLF_POINTER_FRAME_EVENT_NONE = 0x0,
    WLF_POINTER_FRAME_EVENT_STATE = 0x1,
    WLF_POINTER_FRAME_EVENT_ENTER = 0x2,
    WLF_POINTER_FRAME_EVENT_LEAVE = 0x4,
    WLF_POINTER_FRAME_EVENT_BUTTON = 0x8,
    WLF_POINTER_FRAME_EVENT_MOTION_ABSOLUTE = 0x10,
    WLF_POINTER_FRAME_EVENT_MOTION_RELATIVE = 0x20,
    WLF_POINTER_FRAME_EVENT_AXIS_SOURCE = 0x40,
    WLF_POINTER_FRAME_EVENT_AXIS_X = 0x80,
    WLF_POINTER_FRAME_EVENT_AXIS_Y = 0x100,
    WLF_POINTER_FRAME_EVENT_AXIS_X_STOP = 0x200,
    WLF_POINTER_FRAME_EVENT_AXIS_Y_STOP = 0x400,
    WLF_POINTER_FRAME_EVENT_AXIS_X_DISCRETE = 0x800,
    WLF_POINTER_FRAME_EVENT_AXIS_Y_DISCRETE = 0x1000,
    WLF_POINTER_FRAME_EVENT_AXIS_X_DIRECTION = 0x2000,
    WLF_POINTER_FRAME_EVENT_AXIS_Y_DIRECTION = 0x4000,
};

enum wlf_pointer_state : uint32_t {
    WLF_POINTER_STATE_NONE = 0,
    WLF_POINTER_STATE_LOCKED = 1,
    WLF_POINTER_STATE_CONFINED = 2,
};

struct wlf_pointer_frame {
    enum wlf_pointer_frame_event mask;

    uint32_t serial;
    int64_t time;

    enum wlf_pointer_state state;

    struct wlf_surface *enter, *leave;

    struct {
        uint32_t code;
        uint32_t state;
    } button;

    double x, dx, udx;
    double y, dy, udy;

    struct {
        struct {
            double value;
            double discrete;
            uint32_t direction;
        } x, y;
        uint32_t source;
    } axis;
};

struct wlf_pointer_constraint {
    struct wl_list link;
    struct wlf_pointer *pointer;
    struct wlf_surface *surface;
    union {
        struct zwp_locked_pointer_v1 *wp_locked_pointer_v1;
        struct zwp_confined_pointer_v1 *wp_confined_pointer_v1;
    };
    uint32_t type;
    uint32_t lifetime;
};

struct wlf_pointer {
    struct wl_pointer                   *wl_pointer;
    struct zwp_input_timestamps_v1      *wp_timestamps_v1;
    struct zwp_relative_pointer_v1      *wp_relative_v1;
    struct zwp_pointer_gesture_swipe_v1 *wp_swipe_v1;
    struct zwp_pointer_gesture_pinch_v1 *wp_pinch_v1;
    struct zwp_pointer_gesture_hold_v1  *wp_hold_v1;

    struct wl_cursor_theme *wl_cursor_theme;
    struct wl_cursor       *wl_cursor;
    struct wl_surface      *wl_surface;

    struct wlf_pointer_frame frame;

    struct wl_list constraints;
};

struct wlf_keyboard {
    struct wl_keyboard             *wl_keyboard;
    struct zwp_input_timestamps_v1 *wp_timestamps_v1;

    struct xkb_context       *xkb_context;
    struct xkb_keymap        *xkb_keymap;
    struct xkb_state         *xkb_state;
    struct xkb_compose_state *xkb_compose_state;

    const char *locale;

    int32_t repeat_rate;
    int32_t repeat_delay;
    int64_t event_time;
};

struct wlf_touch_point {
    struct wl_list link;
    struct wlf_surface *down;
    double x, y;
    int32_t id;
};

struct wlf_touch_frame {
    uint32_t serial;
    int64_t  time;

    struct wl_list points;
};

struct wlf_touch {
    struct wl_touch                *wl_touch;
    struct zwp_input_timestamps_v1 *wp_timestamps_v1;

    struct wlf_touch_frame frame;
};

struct wlf_clipboard {
    struct wl_data_device *wl_data_device;
};

struct wlf_text_input {
    struct zwp_text_input_v3 *wp_text_input_v3;
};

struct wlf_shortcuts_inhibitor {
    struct wl_list link;
    struct wlf_seat *seat;
    struct wlf_surface *surface;
    struct zwp_keyboard_shortcuts_inhibitor_v1 *wp_shortcuts_inhibitor_v1;
};

struct wlf_seat {
    struct wlf_global global;
    struct wl_list    link;

    struct wlf_seat_listener listener;

    struct wl_seat                  *wl_seat;
    struct ext_idle_notification_v1 *ext_idle_notification_v1;

    struct wlf_pointer    pointer;
    struct wlf_keyboard   keyboard;
    struct wlf_touch      touch;
    struct wlf_clipboard  clipboard;
    struct wlf_text_input text_input;

    struct wl_list shortcut_inhibitors;

    void *user_data;
};

void
wlf_seat_handle_surface_destroyed(struct wlf_seat *seat, struct wlf_surface *surface);

struct wlf_seat *
wlf_seat_add(struct wlf_context *context, uint32_t name, uint32_t version);

void
wlf_seat_remove(struct wlf_seat *seat);

void
wlf_destroy_seats(struct wlf_context *context);
