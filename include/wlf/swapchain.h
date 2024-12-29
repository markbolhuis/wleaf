#pragma once

#include "wlf/common.h"

struct wlf_swapchain;

enum wlf_result
wlf_swapchain_create(
    struct wlf_surface *surface,
    struct wlf_extent extent,
    uint32_t format,
    struct wlf_swapchain **swapchain);

void
wlf_swapchain_destroy(struct wlf_swapchain *swapchain);

int32_t
wlf_swapchain_acquire(struct wlf_swapchain *swapchain);

enum wlf_result
wlf_swapchain_present(struct wlf_swapchain *swapchain, int32_t image);

uint8_t *
wlf_swapchain_get_pixels(struct wlf_swapchain *swapchain, int32_t image);

enum wlf_result
wlf_swapchain_resize(struct wlf_swapchain *swapchain, struct wlf_extent extent);

int32_t
wlf_swapchain_get_stride(struct wlf_swapchain *swapchain);

struct wlf_extent
wlf_swapchain_get_extent(struct wlf_swapchain *swapchain);
