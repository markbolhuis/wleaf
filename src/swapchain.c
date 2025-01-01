#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdckdint.h>
#include <assert.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <wayland-client-protocol.h>

#include "wlf/swapchain.h"

#include "context_priv.h"
#include "surface_priv.h"

constexpr int32_t MAX_IMAGES = 2;
constexpr int32_t MAX_WIDTH = 3000;
constexpr int32_t MAX_HEIGHT = 2000;

enum status {
    AVAILABLE,
    ACQUIRED,
    BUSY,
    OUTDATED,
};

struct wlf_swapchain {
    struct wlf_surface *surface;

    uint32_t format;

    int32_t width;
    int32_t height;

    int32_t stride;
    int32_t buffer_size;
    int32_t max_width;
    int32_t max_height;

    struct wl_shm_pool *pool;
    int mfd;
    void *data;
    int32_t pool_size;

    struct {
        struct wl_buffer *wl_buffer;
        enum status status;
    } images[MAX_IMAGES];
};

static int32_t
get_bpp(uint32_t format)
{
    switch (format) {
        case WL_SHM_FORMAT_RGB888:
        case WL_SHM_FORMAT_BGR888:
            return 3;
        case WL_SHM_FORMAT_ARGB8888:
        case WL_SHM_FORMAT_XRGB8888:
            return 4;
        case WL_SHM_FORMAT_XRGB16161616:
        case WL_SHM_FORMAT_XBGR16161616:
        case WL_SHM_FORMAT_ARGB16161616:
        case WL_SHM_FORMAT_ABGR16161616:
            return 8;
        default:
            return 0;
    }
}

static void
wl_buffer_release(void *data, struct wl_buffer *wl_buffer);

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

static void
wl_buffer_release(void *data, struct wl_buffer *wl_buffer)
{
    struct wlf_swapchain *sc = data;

    int32_t i = 0;
    for (; i < MAX_IMAGES; ++i) {
        if (wl_buffer == sc->images[i].wl_buffer) {
            break;
        }
    }
    assert(i != MAX_IMAGES);
    auto img = &sc->images[i];

    if (img->status == OUTDATED) {
        wl_buffer_destroy(img->wl_buffer);

        img->wl_buffer = wl_shm_pool_create_buffer(
                sc->pool,
                sc->buffer_size * i,
                sc->width,
                sc->height,
                sc->stride,
                sc->format);
        wl_buffer_add_listener(img->wl_buffer, &wl_buffer_listener, data);
    }

    img->status = AVAILABLE;
}

enum wlf_result
create_pool(struct wlf_swapchain *sc, int32_t pool_size)
{
    sc->mfd = memfd_create("wlf_swapchain", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (sc->mfd < 0) {
        return WLF_ERROR_UNKNOWN;
    }

    int r = ftruncate(sc->mfd, pool_size);
    if (r < 0) {
        goto err;
    }

    r = fcntl(sc->mfd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_SEAL);
    if (r < 0) {
        goto err;
    }

    sc->data = mmap(
            nullptr,
            pool_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            sc->mfd,
            0);
    if (sc->data == MAP_FAILED) {
        goto err;
    }

    auto ctx = sc->surface->context;

    sc->pool_size = pool_size;
    sc->pool = wl_shm_create_pool(ctx->wl_shm, sc->mfd, pool_size);

    return WLF_SUCCESS;
err:
    close(sc->mfd);
    return WLF_ERROR_UNKNOWN;
}

static enum wlf_result
wlf_swapchain_init(
    struct wlf_swapchain *sc,
    struct wlf_surface *surface,
    struct wlf_extent extent,
    uint32_t format)
{
    // bytes per pixel
    int32_t bpp = get_bpp(format);
    if (bpp <= 0) {
        return WLF_ERROR_INVALID_ARGUMENT;
    }

    sc->max_width = MAX_WIDTH;
    sc->max_height = MAX_HEIGHT;

    int32_t stride = 0;
    if (ckd_mul(&stride, sc->max_width, bpp)) {
        return WLF_ERROR_INVALID_ARGUMENT;
    }
    int32_t buffer_size = 0;
    if (ckd_mul(&buffer_size, stride, sc->max_height)) {
        return WLF_ERROR_INVALID_ARGUMENT;
    }
    int32_t pool_size = 0;
    if (ckd_mul(&pool_size, buffer_size, MAX_IMAGES)) {
        return WLF_ERROR_INVALID_ARGUMENT;
    }

    sc->surface     = surface;
    sc->format      = format;
    sc->width       = extent.width;
    sc->height      = extent.height;
    sc->stride      = stride;
    sc->buffer_size = buffer_size;

    create_pool(sc, pool_size);

    for (int32_t i = 0; i < MAX_IMAGES; ++i) {
        auto img = sc->images + i;

        img->status = AVAILABLE;
        img->wl_buffer = wl_shm_pool_create_buffer(
            sc->pool,
            sc->buffer_size * i,
            sc->width,
            sc->height,
            sc->stride,
            sc->format);
        wl_buffer_add_listener(img->wl_buffer, &wl_buffer_listener, sc);
    }

    return WLF_SUCCESS;
}

enum wlf_result
wlf_swapchain_create(
    struct wlf_surface *surface,
    struct wlf_extent extent,
    uint32_t format,
    struct wlf_swapchain **swapchain)
{
    struct wlf_swapchain *sc = calloc(1, sizeof(*sc));
    if (!sc) {
        return WLF_ERROR_OUT_OF_MEMORY;
    }

    auto res = wlf_swapchain_init(sc, surface, extent, format);
    if (res >= WLF_SUCCESS) {
        *swapchain = sc;
    } else {
        free(sc);
    }

    return res;
}

void
wlf_swapchain_destroy(struct wlf_swapchain *sc)
{
    wlf_swapchain_present(sc, -1);

    for (int32_t i = 0; i < MAX_IMAGES; ++i) {
        wl_buffer_destroy(sc->images[i].wl_buffer);
    }

    wl_shm_pool_destroy(sc->pool);
    munmap(sc->data, sc->pool_size);
    close(sc->mfd);

    free(sc);
}

int32_t
wlf_swapchain_acquire(struct wlf_swapchain *sc)
{
    for (int32_t i = 0; i < MAX_IMAGES; ++i) {
        if (sc->images[i].status == AVAILABLE) {
            sc->images[i].status = ACQUIRED;
            return i;
        }
    }
    return -1;
}

enum wlf_result
wlf_swapchain_present(struct wlf_swapchain *sc, int32_t image)
{
    auto s = sc->surface;

    if (image < 0) {
        wl_surface_attach(s->wl_surface, nullptr, 0, 0);
        wl_surface_commit(s->wl_surface);
        return WLF_SUCCESS;
    }

    auto img = &sc->images[image];

    if (image >= MAX_IMAGES || img->status != ACQUIRED) {
        return WLF_ERROR_INVALID_ARGUMENT;
    }

    wl_surface_attach(s->wl_surface, img->wl_buffer, 0, 0);
    if (wl_surface_get_version(s->wl_surface) >= WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION) {
        wl_surface_damage_buffer(s->wl_surface, 0, 0, sc->width, sc->height);
    } else {
        wl_surface_damage(s->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
    }
    wl_surface_commit(s->wl_surface);

    img->status = BUSY;

    return WLF_SUCCESS;
}

uint8_t *
wlf_swapchain_get_pixels(struct wlf_swapchain *sc, int32_t image)
{
    if (image < 0 || image >= MAX_IMAGES) {
        return nullptr;
    }

    if (sc->images[image].status != ACQUIRED) {
        return nullptr;
    }

    return ((uint8_t *)sc->data) + (sc->buffer_size * image);
}

enum wlf_result
wlf_swapchain_resize(struct wlf_swapchain *sc, struct wlf_extent extent)
{
    if (extent.width > sc->max_width) {
        extent.width = sc->max_width;
    }
    if (extent.height > sc->max_height) {
        extent.height = sc->max_height;
    }

    if (extent.width == sc->width &&
        extent.height == sc->height)
    {
        return WLF_SKIPPED;
    }

    sc->width = extent.width;
    sc->height = extent.height;

    for (int32_t i = 0; i < MAX_IMAGES; ++i) {
        auto img = &sc->images[i];

        if (img->status == OUTDATED || img->status == BUSY) {
            img->status = OUTDATED;
            continue;
        }

        wl_buffer_destroy(img->wl_buffer);
        img->wl_buffer = wl_shm_pool_create_buffer(
                sc->pool,
                sc->buffer_size * i,
                sc->width,
                sc->height,
                sc->stride,
                sc->format);
        wl_buffer_add_listener(
                img->wl_buffer,
                &wl_buffer_listener,
                sc);
    }

    return WLF_SUCCESS;
}

int32_t
wlf_swapchain_get_stride(struct wlf_swapchain *sc)
{
    return sc->stride;
}

struct wlf_extent
wlf_swapchain_get_extent(struct wlf_swapchain *sc)
{
    return (struct wlf_extent) {
        .width = sc->width,
        .height = sc->height,
    };
}
