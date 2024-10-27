#pragma once

#include "wlf/common.h"

struct wlf_output {
    struct wlf_global global;

    struct wl_list link;

    struct wl_output *wl_output;
    struct zxdg_output_v1 *xdg_output_v1;
    int done_count;

    struct wlf_offset pos;
    struct wlf_extent logical;
    struct wlf_extent pixel;
    struct wlf_extent physical;
    enum wlf_transform transform;
    int32_t scale;
    int32_t subpixel;
    int32_t refresh;

    char *name;
    char *description;
    char *make;
    char *model;
};

struct wlf_output_ref {
    struct wl_list link;
    struct wlf_output *output;
};

int32_t
wlf_output_get_scale(struct wlf_output *output);

int32_t
wlf_output_ref_list_get_max_scale(const struct wl_list *list);

struct wlf_output_ref *
wlf_output_ref_list_find(const struct wl_list *list, struct wlf_output *output);

bool
wlf_output_ref_list_add(struct wl_list *list, struct wlf_output *output);

bool
wlf_output_ref_list_remove(struct wl_list *list, struct wlf_output *output);

void
wlf_output_init_xdg(struct wlf_output *output);

void
wlf_output_init_xdg_all(struct wlf_context *context);

struct wlf_output *
wlf_output_bind(struct wlf_context *context, uint32_t name, uint32_t version);

void
wlf_output_destroy(struct wlf_output *output);

void
wlf_destroy_outputs(struct wlf_context *context);
