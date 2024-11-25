#pragma once

#include "wlf/common.h"

enum wlf_input_type : uint32_t {
    WLF_INPUT_TYPE_NONE = 0,
    WLF_INPUT_TYPE_POINTER = 1,
    WLF_INPUT_TYPE_KEYBOARD = 2,
    WLF_INPUT_TYPE_TOUCH = 4,
    WLF_INPUT_TYPE_CLIPBOARD = 8,
    WLF_INPUT_TYPE_GESTURES = 16,
    WLF_INPUT_TYPE_TEXT = 32,
    WLF_INPUT_TYPE_TABLET = 64,
};

enum wlf_cursor : uint32_t {
    WLF_CURSOR_HIDDEN = 0,
    WLF_CURSOR_DEFAULT = 1,
    WLF_CURSOR_CONTEXT_MENU = 2,
    WLF_CURSOR_HELP = 3,
    WLF_CURSOR_POINTER = 4,
    WLF_CURSOR_PROGRESS = 5,
    WLF_CURSOR_WAIT = 6,
    WLF_CURSOR_CELL = 7,
    WLF_CURSOR_CROSSHAIR = 8,
    WLF_CURSOR_TEXT = 9,
    WLF_CURSOR_VERTICAL_TEXT = 10,
    WLF_CURSOR_ALIAS = 11,
    WLF_CURSOR_COPY = 12,
    WLF_CURSOR_MOVE = 13,
    WLF_CURSOR_NO_DROP = 14,
    WLF_CURSOR_NOT_ALLOWED = 15,
    WLF_CURSOR_GRAB = 16,
    WLF_CURSOR_GRABBING = 17,
    WLF_CURSOR_E_RESIZE = 18,
    WLF_CURSOR_N_RESIZE = 19,
    WLF_CURSOR_NE_RESIZE = 20,
    WLF_CURSOR_NW_RESIZE = 21,
    WLF_CURSOR_S_RESIZE = 22,
    WLF_CURSOR_SE_RESIZE = 23,
    WLF_CURSOR_SW_RESIZE = 24,
    WLF_CURSOR_W_RESIZE = 25,
    WLF_CURSOR_EW_RESIZE = 26,
    WLF_CURSOR_NS_RESIZE = 27,
    WLF_CURSOR_NESW_RESIZE = 28,
    WLF_CURSOR_NWSE_RESIZE = 29,
    WLF_CURSOR_COL_RESIZE = 30,
    WLF_CURSOR_ROW_RESIZE = 31,
    WLF_CURSOR_ALL_SCROLL = 32,
    WLF_CURSOR_ZOOM_IN = 33,
    WLF_CURSOR_ZOOM_OUT = 34,
};

enum wlf_gesture : uint32_t {
    WLF_GESTURE_SWIPE = 0,
    WLF_GESTURE_PINCH = 1,
    WLF_GESTURE_HOLD = 2,
};

enum wlf_key_state : uint32_t {
    WLF_KEY_STATE_RELEASED = 0,
    WLF_KEY_STATE_PRESSED = 1,
    WLF_KEY_STATE_REPEATED = 2,
};

struct wlf_seat_listener {
    void (*lost)(void *user_data);
    void (*name)(void *user_data, const char8_t *name);
    void (*idled)(void *user_data, bool idled);
    void (*shortcuts_inhibited)(void *user_data, struct wlf_surface *surface, bool inhibited);
};

struct wlf_seat_info {
    uint64_t id;
    void *user_data;
};

enum wlf_result
wlf_seat_bind(
    struct wlf_context *context,
    const struct wlf_seat_info *info,
    const struct wlf_seat_listener *listener,
    struct wlf_seat **seat);

void
wlf_seat_release(struct wlf_seat *seat);

enum wlf_result
wlf_seat_set_idle_time(struct wlf_seat *seat, int64_t time);

enum wlf_result
wlf_seat_inhibit_shortcuts(struct wlf_seat *seat, struct wlf_surface *surface, bool inhibit);
