#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <wayland-client-protocol.h>
#include <xdg-output-unstable-v1-client-protocol.h>

#include "context_priv.h"
#include "output_priv.h"
#include "surface_priv.h"

extern const uint32_t WLF_WL_OUTPUT_VERSION;
static constexpr uint32_t WLF_XDG_OUTPUT_V1_DONE_DEPRECATED_SINCE_VERSION = 3;

int32_t
wlf_output_get_scale(struct wlf_output *output)
{
    if (wl_output_get_version(output->wl_output) < WL_OUTPUT_SCALE_SINCE_VERSION) {
        return 1;
    }

    return output->scale;
}

int32_t
wlf_output_ref_list_get_max_scale(const struct wl_list *list)
{
    int32_t max_scale = 1;

    struct wlf_output_ref *ref;
    wl_list_for_each(ref, list, link) {
        int32_t scale = wlf_output_get_scale(ref->output);
        if (scale > max_scale) {
            max_scale = scale;
        }
    }

    return max_scale;
}

struct wlf_output_ref *
wlf_output_ref_list_find(const struct wl_list *list, struct wlf_output *output)
{
    struct wlf_output_ref *ref;
    wl_list_for_each(ref, list, link) {
        if (ref->output == output) {
            return ref;
        }
    }

    return nullptr;
}

bool
wlf_output_ref_list_add(struct wl_list *list, struct wlf_output *output)
{
    struct wlf_output_ref *ref = wlf_output_ref_list_find(list, output);
    if (ref) {
        return false;
    }

    ref = calloc(1, sizeof(*ref));
    if (!ref) {
        return false;
    }

    ref->output = output;
    wl_list_insert(list, &ref->link);
    return true;
}

bool
wlf_output_ref_list_remove(struct wl_list *list, struct wlf_output *output)
{
    struct wlf_output_ref *ref = wlf_output_ref_list_find(list, output);
    if (!ref) {
        return false;
    }

    wl_list_remove(&ref->link);
    free(ref);
    return true;
}

static int
wlf_output_get_required_done_count(struct wlf_output *output)
{
    int count = 0;

    uint32_t wl_ver = wl_output_get_version(output->wl_output);
    if (wl_ver >= WL_OUTPUT_DONE_SINCE_VERSION) {
        count++;
    }

    if (!output->xdg_output_v1) {
        return count;
    }

    uint32_t xdg_ver = zxdg_output_v1_get_version(output->xdg_output_v1);
    if (xdg_ver < WLF_XDG_OUTPUT_V1_DONE_DEPRECATED_SINCE_VERSION ||
        wl_ver >= WL_OUTPUT_DONE_SINCE_VERSION)
    {
        count++;
    }

    return count;
}

static void
wlf_output_done(struct wlf_output *output)
{
    int required = wlf_output_get_required_done_count(output);
    if (output->done_count < required) {
        return;
    }

    // TODO: handle atomic update
}

// region XDG Output V1

static void
xdg_output_v1_logical_position(void *data, struct zxdg_output_v1 *, int32_t x, int32_t y)
{
    struct wlf_output *output = data;

    output->pos.x = x;
    output->pos.y = y;
}

static void
xdg_output_v1_logical_size(void *data, struct zxdg_output_v1 *, int32_t width, int32_t height)
{
    struct wlf_output *output = data;

    output->logical.width = width;
    output->logical.height = height;
}

static void
xdg_output_v1_done(void *data, struct zxdg_output_v1 *xdg_output_v1)
{
    struct wlf_output *output = data;

    // Catch any compositor bug, and prevent it messing up the done count
    if (zxdg_output_v1_get_version(xdg_output_v1) >=
        WLF_XDG_OUTPUT_V1_DONE_DEPRECATED_SINCE_VERSION)
    {
        return;
    }

    output->done_count++;
    wlf_output_done(output);
}

static void
xdg_output_v1_name(void *data, struct zxdg_output_v1 *, const char *name)
{
    struct wlf_output *output = data;

    if (wl_output_get_version(output->wl_output) >= WL_OUTPUT_NAME_SINCE_VERSION) {
        return;
    }

    free(output->name);
    output->name = strdup(name);
}

static void
xdg_output_v1_description(void *data, struct zxdg_output_v1 *, const char *description)
{
    struct wlf_output *output = data;

    if (wl_output_get_version(output->wl_output) >= WL_OUTPUT_DESCRIPTION_SINCE_VERSION) {
        return;
    }

    free(output->description);
    output->description = strdup(description);
}

static const struct zxdg_output_v1_listener xdg_output_v1_listener = {
    .logical_position = xdg_output_v1_logical_position,
    .logical_size     = xdg_output_v1_logical_size,
    .done             = xdg_output_v1_done,
    .name             = xdg_output_v1_name,
    .description      = xdg_output_v1_description,
};

// endregion

// region WL Output

static void
wl_output_geometry(
    void *data,
    struct wl_output *,
    int32_t x,
    int32_t y,
    int32_t physical_width,
    int32_t physical_height,
    int32_t subpixel,
    const char *make,
    const char *model,
    int32_t transform)
{
    struct wlf_output *output = data;

    if (!output->xdg_output_v1) {
        output->pos.x = x;
        output->pos.y = y;
    }

    output->physical.width = physical_width;
    output->physical.height = physical_height;
    output->subpixel = subpixel;

    free(output->make);
    output->make = strdup(make);

    free(output->model);
    output->model = strdup(model);

    output->transform = (enum wlf_transform)transform;
}

static void
wl_output_mode(
    void *data,
    struct wl_output *,
    uint32_t flags,
    int32_t width,
    int32_t height,
    int32_t refresh)
{
    struct wlf_output *output = data;

    if (flags != WL_OUTPUT_MODE_CURRENT) {
        return;
    }

    output->pixel.width = width;
    output->pixel.height = height;

    output->refresh = refresh;
}

static void
wl_output_done(void *data, struct wl_output *)
{
    struct wlf_output *output = data;
    if (output->xdg_output_v1) {
        output->done_count++;
    } else {
        output->done_count = 1;
    }
    wlf_output_done(output);
}

static void
wl_output_scale(void *data, struct wl_output *, int32_t scale)
{
    struct wlf_output *output = data;
    output->scale = scale;
}

static void
wl_output_name(void *data, struct wl_output *, const char *name)
{
    struct wlf_output *output = data;

    free(output->name);
    output->name = strdup(name);
}

static void
wl_output_description(void *data, struct wl_output *, const char *description)
{
    struct wlf_output *output = data;

    free(output->description);
    output->description = strdup(description);
}

static const struct wl_output_listener wl_output_listener = {
    .geometry    = wl_output_geometry,
    .mode        = wl_output_mode,
    .done        = wl_output_done,
    .scale       = wl_output_scale,
    .name        = wl_output_name,
    .description = wl_output_description,
};

// endregion

void
wlf_output_init_xdg(struct wlf_output *output)
{
    struct wlf_context *context = output->global.context;

    assert(context->xdg_output_manager_v1);
    assert(!output->xdg_output_v1);

    output->xdg_output_v1 = zxdg_output_manager_v1_get_xdg_output(
        context->xdg_output_manager_v1, output->wl_output);
    zxdg_output_v1_add_listener( output->xdg_output_v1, &xdg_output_v1_listener, output);
}

void
wlf_output_init_xdg_all(struct wlf_context *context)
{
    assert(context->xdg_output_manager_v1);

    struct wlf_output *output;
    wl_list_for_each(output, &context->output_list, link) {
        if (!output->xdg_output_v1) {
            wlf_output_init_xdg(output);
        }
    }
}

struct wlf_output *
wlf_output_bind(struct wlf_context *context, uint32_t name, uint32_t version)
{
    struct wlf_output *output = calloc(1, sizeof(struct wlf_output));
    if (!output) {
        return nullptr;
    }

    output->global.context = context;
    output->global.id = wlf_new_id(context);
    output->global.name = name;
    output->global.version = version;

    version = wlf_get_version(&wl_output_interface, version, WLF_WL_OUTPUT_VERSION);

    output->wl_output = wl_registry_bind(
        context->wl_registry, name, &wl_output_interface, version);
    wl_output_add_listener(output->wl_output, &wl_output_listener, output);

    if (context->xdg_output_manager_v1) {
        wlf_output_init_xdg(output);
    }

    wl_list_insert(&context->output_list, &output->link);
    return output;
}

void
wlf_output_destroy(struct wlf_output *output)
{
    struct wlf_context *context = output->global.context;

    struct wlf_surface *surface;
    wl_list_for_each(surface, &context->surface_list, link) {
        wlf_surface_handle_output_destroyed(surface, output);
    }

    if (output->xdg_output_v1) {
        zxdg_output_v1_destroy(output->xdg_output_v1);
    }

    if (wl_output_get_version(output->wl_output) >=
        WL_OUTPUT_RELEASE_SINCE_VERSION)
    {
        wl_output_release(output->wl_output);
    } else {
        wl_output_destroy(output->wl_output);
    }

    free(output->name);
    free(output->description);
    free(output->make);
    free(output->model);

    wl_list_remove(&output->link);
    free(output);
}

void
wlf_destroy_outputs(struct wlf_context *context)
{
    struct wlf_output *output, *output_tmp;
    wl_list_for_each_safe(output, output_tmp, &context->output_list, link) {
        wlf_output_destroy(output);
    }
}
