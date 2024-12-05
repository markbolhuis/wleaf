#pragma once

#include "common.h"

void
wlf_surface_set_user_data(struct wlf_surface *surface, void *data);

void *
wlf_surface_get_user_data(struct wlf_surface *surface);

struct wlf_offset
wlf_surface_point_to_buffer_offset(struct wlf_surface *surface, struct wlf_point point);

enum wlf_result
wlf_surface_inhibit_idling(struct wlf_surface *surface, bool enable);

enum wlf_result
wlf_surface_set_content_type(struct wlf_surface *surface, enum wlf_content_type type);

enum wlf_result
wlf_surface_set_alpha_multiplier(struct wlf_surface *surface, uint32_t factor);
