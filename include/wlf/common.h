#pragma once

#include <stdint.h>
#include <uchar.h>

struct wlf_context;
struct wlf_seat;
struct wlf_surface;
struct wlf_toplevel;
struct wlf_popup;

enum wlf_result : int32_t {
    WLF_SUCCESS = 0,
    WLF_ALREADY_SET = 1,
    WLF_SKIPPED = 2,
    WLF_ERROR_UNKNOWN = -1,
    WLF_ERROR_OUT_OF_MEMORY = -2,
    WLF_ERROR_UNSUPPORTED = -3,
    WLF_ERROR_WAYLAND = -4,
    WLF_ERROR_UNINITIALIZED = -5,
    WLF_ERROR_INVALID_ARGUMENT = -6,
    WLF_ERROR_LOST = -7,
};

struct wlf_offset {
    int32_t x;
    int32_t y;
};

struct wlf_extent {
    int32_t width;
    int32_t height;
};

struct wlf_rect {
    struct wlf_offset offset;
    struct wlf_extent extent;
};

enum wlf_edge : uint32_t {
    WLF_EDGE_NONE = 0,
    WLF_EDGE_TOP = 1,
    WLF_EDGE_BOTTOM = 2,
    WLF_EDGE_LEFT = 4,
    WLF_EDGE_TOP_LEFT = 5,
    WLF_EDGE_BOTTOM_LEFT = 6,
    WLF_EDGE_RIGHT = 8,
    WLF_EDGE_TOP_RIGHT = 9,
    WLF_EDGE_BOTTOM_RIGHT = 10,
};

enum wlf_adjust : uint32_t {
    WLF_ADJUST_NONE = 0,
    WLF_ADJUST_SLIDE_X = 1,
    WLF_ADJUST_SLIDE_Y = 2,
    WLF_ADJUST_FLIP_X = 4,
    WLF_ADJUST_FLIP_Y = 8,
    WLF_ADJUST_RESIZE_X = 16,
    WLF_ADJUST_RESIZE_Y = 32,
};

enum wlf_transform : uint32_t {
    WLF_TRANSFORM_NONE = 0,
    WLF_TRANSFORM_90 = 1,
    WLF_TRANSFORM_180 = 2,
    WLF_TRANSFORM_270 = 3,
    WLF_TRANSFORM_FLIPPED = 4,
    WLF_TRANSFORM_FLIPPED_90 = 5,
    WLF_TRANSFORM_FLIPPED_180 = 6,
    WLF_TRANSFORM_FLIPPED_270 = 7,
};

enum wlf_content_type : uint32_t {
    WLF_CONTENT_TYPE_NONE = 0,
    WLF_CONTENT_TYPE_PHOTO = 1,
    WLF_CONTENT_TYPE_VIDEO = 2,
    WLF_CONTENT_TYPE_GAME = 3,
};
