#pragma once

#include "wlf/common.h"

static inline bool
wlf_extent_equal(struct wlf_extent a, struct wlf_extent b)
{
    return a.width == b.width && a.height == b.height;
}

static inline int32_t
wlf_div_round(int32_t a, int32_t b)
{
    return (a + b / 2) / b;
}

[[maybe_unused]]
static struct wlf_extent
wlf_max_extent_with_aspect(struct wlf_extent e, struct wlf_extent a)
{
    int32_t lhs = e.width * a.height;
    int32_t rhs = e.height * a.width;
    if (lhs > rhs) {
        e.width = rhs / a.height;
    } else {
        e.height = lhs / a.width;
    }
    return e;
}

static inline int64_t
wlf_tv_to_ns(uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec)
{
    int64_t tv_sec = ((int64_t)tv_sec_hi << 32) | (int64_t)tv_sec_lo;
    return (tv_sec * 1000000000) + (int64_t)tv_nsec;
}

static inline int64_t
wlf_us_to_ns(uint32_t utime_hi, uint32_t utime_lo)
{
    return (((int64_t)utime_hi) << 32 | (int64_t)utime_lo) * 1000;
}

static inline int64_t
wlf_ms_to_ns(uint32_t mtime)
{
    return ((int64_t)mtime) * 1000000;
}

static inline bool
wlf_transform_is_vertical(enum wlf_transform transform)
{
    return (transform & WLF_TRANSFORM_90) == WLF_TRANSFORM_90;
}

static inline bool
wlf_transform_is_perpendicular(enum wlf_transform before, enum wlf_transform after)
{
    return ((before ^ after) & WLF_TRANSFORM_90) == WLF_TRANSFORM_90;
}

[[maybe_unused]]
static enum wlf_transform
wlf_transform_inverse(enum wlf_transform t)
{
    return (t ^ WLF_TRANSFORM_90) & WLF_TRANSFORM_FLIPPED_90 ? t : t ^ WLF_TRANSFORM_180;
}
