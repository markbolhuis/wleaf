#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include "wlf/context.h"
#include "wlf/surface.h"
#include "wlf/input.h"
#include "wlf/toplevel.h"
#include "wlf/swapchain.h"

#define PATTERN 0

struct window {
    struct wlf_toplevel  *toplevel;
    struct wlf_swapchain *swapchain;

    struct wlf_extent extent;
    bool resized;
    bool closed;
};

struct demo {
    struct wlf_context *context;
    uint64_t seat_id;
    struct wlf_seat *seat;
};

static void
on_toplevel_configure(void *data, struct wlf_extent extent)
{
    struct window *win = data;
    if (!win->swapchain) {
        auto s = wlf_toplevel_get_surface(win->toplevel);
        auto r = wlf_swapchain_create(s, extent, 0, &win->swapchain);
        assert(r >= WLF_SUCCESS);
    } else {
        win->resized = true;
    }
    win->extent = extent;
}

static void
on_toplevel_close(void *data)
{
    struct window *win = data;
    win->closed = true;
}

static const struct wlf_toplevel_listener toplevel_listener = {
    .configure = on_toplevel_configure,
    .close     = on_toplevel_close,
};

static void
on_seat_name(void *data, const char8_t *name) { }

static void
on_seat_idled(void *data, bool idle) { }

static void
on_seat_shortcuts_inhibited(void *data, struct wlf_surface *s, bool inhibited) { }

static const struct wlf_seat_listener seat_listener = {
    .name                = on_seat_name,
    .idled               = on_seat_idled,
    .shortcuts_inhibited = on_seat_shortcuts_inhibited,
};

static void
on_seat(void *data, uint64_t id, bool added)
{
    struct demo *demo = data;
    if (added) {
        if (demo->seat_id == 0) {
            demo->seat_id = id;
        }
    } else if (demo->seat && demo->seat_id == id) {
        wlf_seat_release(demo->seat);
        demo->seat = nullptr;
        demo->seat_id = 0;
    }
}

static const struct wlf_context_listener context_listener = {
    .seat = on_seat,
};

static void
window_draw(struct window *win, int32_t image)
{
    struct wlf_extent extent = wlf_swapchain_get_extent(win->swapchain);
    int32_t stride = wlf_swapchain_get_stride(win->swapchain);
    uint8_t *pixels = wlf_swapchain_get_pixels(win->swapchain, image);

#if PATTERN == 0

    for (int y = 0; y < extent.height; ++y) {
        for (int x = 0; x < extent.width; ++x) {
            int32_t n = (y * stride) + (x * 4);

            if (x % 200 < 1 || y % 200 < 1) {
                pixels[n + 0] = 0xFF;
                pixels[n + 1] = 0xFF;
                pixels[n + 2] = 0xFF;
                pixels[n + 3] = 0xFF;
            } else {
                pixels[n + 0] = 0x00;
                pixels[n + 1] = 0x00;
                pixels[n + 2] = 0x00;
                pixels[n + 3] = 0x77;
            }
        }
    }

#elif PATTERN == 1

    for (int y = 0; y < extent.height; ++y) {
        for (int x = 0; x < extent.width; ++x) {
            int32_t n = (y * stride) + (x * 4);

            if (y % 200 < 100) {
                if (x % 200 < 100) {
                    pixels[n + 0] = 0xFF;
                    pixels[n + 1] = 0x00;
                    pixels[n + 2] = 0x00;
                    pixels[n + 3] = 0xff;
                } else {
                    pixels[n + 0] = 0x00;
                    pixels[n + 1] = 0xFF;
                    pixels[n + 2] = 0x00;
                    pixels[n + 3] = 0xff;
                }
            } else {
                if (x % 200 < 100) {
                    pixels[n + 0] = 0x00;
                    pixels[n + 1] = 0x00;
                    pixels[n + 2] = 0xFF;
                    pixels[n + 3] = 0xff;
                } else {
                    pixels[n + 0] = 0x00;
                    pixels[n + 1] = 0x00;
                    pixels[n + 2] = 0x00;
                    pixels[n + 3] = 0xff;
                }
            }
        }
    }

#elif PATTERN == 2

    for (int y = 0; y < extent.height; ++y) {
        for (int x = 0; x < extent.width; ++x) {
            int32_t n = (y * stride) + (x * 4);

            int32_t cx = ((x / 100) * 100) + 50;
            int32_t cy = ((y / 100) * 100) + 50;

            if (hypot(cx - x, cy - y) < 45.0) {
                if (y % 200 < 100) {
                    if (x % 200 < 100) {
                        pixels[n + 0] = 0xff;
                        pixels[n + 1] = 0x00;
                        pixels[n + 2] = 0x00;
                        pixels[n + 3] = 0xff;
                    } else {
                        pixels[n + 0] = 0xff;
                        pixels[n + 1] = 0xff;
                        pixels[n + 2] = 0xff;
                        pixels[n + 3] = 0xff;
                    }
                } else {
                    if (x % 200 < 100) {
                        pixels[n + 0] = 0x00;
                        pixels[n + 1] = 0xff;
                        pixels[n + 2] = 0x00;
                        pixels[n + 3] = 0xff;
                    } else {
                        pixels[n + 0] = 0x00;
                        pixels[n + 1] = 0x00;
                        pixels[n + 2] = 0xff;
                        pixels[n + 3] = 0xff;
                    }
                }
            } else {
                pixels[n + 0] = 0x00;
                pixels[n + 1] = 0x00;
                pixels[n + 2] = 0x00;
                pixels[n + 3] = 0xff;
            }
        }
    }

#else
#error No Pattern
#endif
}

struct window *
window_create(struct demo *demo)
{
    struct window *win = calloc(1, sizeof(*win));
    if (!win) {
        return nullptr;
    }

    const char8_t title[] = u8"wlf-demo";
    const char8_t app_id[] = u8"wleaf.demo";

    struct wlf_toplevel_info tl_info = {
        .flags           = WLF_TOPLEVEL_FLAGS_NONE,
        .extent          = { 640, 360 },
        .decoration_mode = WLF_DECORATION_MODE_NO_PREFERENCE,
        .content_type    = WLF_CONTENT_TYPE_NONE,
        .parent          = nullptr,
        .title           = title,
        .app_id          = app_id,
        .modal           = false,
        .user_data       = win,
    };
    enum wlf_result res = wlf_toplevel_create(
        demo->context,
        &tl_info,
        &toplevel_listener,
        &win->toplevel);
    if (res >= WLF_SUCCESS) {
        return win;
    }

    free(win);
    return nullptr;
}

void
window_destroy(struct window *win)
{
    if (win->swapchain) {
        wlf_swapchain_destroy(win->swapchain);
    }

    if (win->toplevel) {
        wlf_toplevel_destroy(win->toplevel);
    }

    free(win);
}

int
main()
{
    struct demo demo = {};

    struct wlf_context_info info = {
        .flags = WLF_CONTEXT_FLAGS_NONE,
        .display = nullptr,
        .user_data = &demo,
    };
    enum wlf_result res = wlf_context_create(&info, &context_listener, &demo.context);
    if (res < WLF_SUCCESS) {
        return 0;
    }

    if (demo.seat_id != 0) {
        struct wlf_seat_info sinfo = {
            .id = demo.seat_id,
            .user_data = &demo,
        };
        res = wlf_seat_bind(demo.context, &sinfo, &seat_listener, &demo.seat);
        if (res < WLF_SUCCESS) {
            goto err_seat;
        }
    }

    struct window *win = window_create(&demo);
    if (!win) {
        goto err_win;
    }

    while (!win->closed) {
        res = wlf_dispatch_events(demo.context, -1);
        if (res < WLF_SUCCESS) {
            break;
        }

        if (win->swapchain) {
            if (win->resized) {
                wlf_swapchain_resize(win->swapchain, win->extent);
                win->resized = false;
            }

            int32_t image = wlf_swapchain_acquire(win->swapchain);
            if (image < 0) {
                continue;
            }

            window_draw(win, image);
            wlf_swapchain_present(win->swapchain, image);
        }
    }

    window_destroy(win);
err_win:
    if (demo.seat) {
        wlf_seat_release(demo.seat);
    }
err_seat:
    wlf_context_destroy(demo.context);
    return 0;
}
