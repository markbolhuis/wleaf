#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <poll.h>
#include <errno.h>

#include <wayland-client-protocol.h>
#include <viewporter-client-protocol.h>
#include <fractional-scale-v1-client-protocol.h>
#include <input-timestamps-unstable-v1-client-protocol.h>
#include <relative-pointer-unstable-v1-client-protocol.h>
#include <pointer-constraints-unstable-v1-client-protocol.h>
#include <pointer-gestures-unstable-v1-client-protocol.h>
#include <keyboard-shortcuts-inhibit-unstable-v1-client-protocol.h>
#include <text-input-unstable-v3-client-protocol.h>
#include <idle-inhibit-unstable-v1-client-protocol.h>
#include <single-pixel-buffer-v1-client-protocol.h>
#include <content-type-v1-client-protocol.h>
#include <alpha-modifier-v1-client-protocol.h>
#include <xdg-shell-client-protocol.h>
#include <xdg-output-unstable-v1-client-protocol.h>
#include <xdg-decoration-unstable-v1-client-protocol.h>
#include <xdg-dialog-v1-client-protocol.h>
#include <ext-idle-notify-v1-client-protocol.h>

#include <xkbcommon/xkbcommon.h>

#include "context_priv.h"
#include "input_priv.h"
#include "output_priv.h"

const uint32_t WLF_WL_COMPOSITOR_VERSION = 6;
const uint32_t WLF_WL_SUBCOMPOSITOR_VERSION = 1;
const uint32_t WLF_WL_SHM_VERSION = 2;
const uint32_t WLF_WL_SEAT_VERSION = 9;
const uint32_t WLF_WL_OUTPUT_VERSION = 4;
const uint32_t WLF_WL_DATA_DEVICE_MANAGER_VERSION = 3;
const uint32_t WLF_WP_TEXT_INPUT_MANAGER_V3_VERSION = 1;
const uint32_t WLF_WP_VIEWPORTER_VERSION = 1;
const uint32_t WLF_WP_FRACTIONAL_SCALE_MANAGER_V1_VERSION = 1;
const uint32_t WLF_WP_INPUT_TIMESTAMPS_MANAGER_V1_VERSION = 1;
const uint32_t WLF_WP_RELATIVE_POINTER_MANAGER_V1_VERSION = 1;
const uint32_t WLF_WP_POINTER_CONSTRAINTS_V1_VERSION = 1;
const uint32_t WLF_WP_POINTER_GESTURES_V1_VERSION = 3;
const uint32_t WLF_WP_KEYBOARD_SHORTCUTS_INHIBIT_MANAGER_V1_VERSION = 1;
const uint32_t WLF_WP_IDLE_INHIBIT_MANAGER_V1_VERSION = 1;
const uint32_t WLF_WP_CONTENT_TYPE_MANAGER_V1_VERSION = 1;
const uint32_t WLF_WP_SINGLE_PIXEL_BUFFER_MANAGER_V1_VERSION = 1;
const uint32_t WLF_WP_ALPHA_MODIFIER_V1_VERSION = 1;
const uint32_t WLF_XDG_WM_BASE_VERSION = 6;
const uint32_t WLF_XDG_WM_DIALOG_V1_VERSION = 1;
const uint32_t WLF_XDG_OUTPUT_MANAGER_V1_VERSION = 3;
const uint32_t WLF_XDG_DECORATION_MANAGER_V1_VERSION = 1;
const uint32_t WLF_EXT_IDLE_NOTIFICATION_V1_VERSION = 1;

uint64_t
wlf_new_id(struct wlf_context *context)
{
    uint64_t id = ++context->id;
    assert(id != 0);
    return id;
}

uint32_t
wlf_get_version(const struct wl_interface *interface, uint32_t version, uint32_t max)
{
    if (version > max) {
        version = max;
    }
    if (version > (uint32_t)interface->version) {
        version = (uint32_t)interface->version;
    }
    return version;
}

static struct wlf_global *
wlf_global_create(struct wlf_context *context, uint32_t name, uint32_t version)
{
    struct wlf_global *global = calloc(1, sizeof(struct wlf_global));
    if (!global) {
        return nullptr;
    }

    global->context = context;
    global->id = wlf_new_id(context);
    global->name = name;
    global->version = version;

    return global;
}

static void
wlf_global_destroy(struct wlf_global *global)
{
    assert(global);
    free(global);
}

static int
wlf_flush(struct wl_display *display)
{
    while (wl_display_flush(display) < 0) {
        if (errno == EINTR) {
            continue;
        }

        if (errno != EAGAIN) {
            return -1;
        }

        struct pollfd fds[1];
        fds[0].fd = wl_display_get_fd(display);
        fds[0].events = POLLOUT;

        int n;
        do {
            n = poll(fds, 1, -1);
        } while (n < 0 && errno == EINTR);

        if (n < 0) {
            return n;
        }
    }

    return 0;
}

static void
wlf_destroy_globals(struct wlf_context *context)
{
#define WLF_GLOBAL_DESTROY(name, prefix) \
    if (context->name) { \
        struct wlf_global *global = prefix##name##_get_user_data(context->name); \
        wlf_global_destroy(global); \
        prefix##name##_destroy(context->name); \
        context->name = nullptr; \
    }

    WLF_GLOBAL_DESTROY(wl_compositor,)
    WLF_GLOBAL_DESTROY(wl_subcompositor,)
    WLF_GLOBAL_DESTROY(wl_data_device_manager,)
    WLF_GLOBAL_DESTROY(wp_viewporter,)
    WLF_GLOBAL_DESTROY(wp_fractional_scale_manager_v1,)
    WLF_GLOBAL_DESTROY(wp_content_type_manager_v1,)
    WLF_GLOBAL_DESTROY(wp_single_pixel_buffer_manager_v1,)
    WLF_GLOBAL_DESTROY(wp_alpha_modifier_v1,)
    WLF_GLOBAL_DESTROY(xdg_wm_base,)
    WLF_GLOBAL_DESTROY(xdg_wm_dialog_v1,)
    WLF_GLOBAL_DESTROY(ext_idle_notifier_v1,)

    WLF_GLOBAL_DESTROY(wp_input_timestamps_manager_v1, z)
    WLF_GLOBAL_DESTROY(wp_relative_pointer_manager_v1, z)
    WLF_GLOBAL_DESTROY(wp_pointer_constraints_v1, z)
    WLF_GLOBAL_DESTROY(wp_keyboard_shortcuts_inhibit_manager_v1, z)
    WLF_GLOBAL_DESTROY(wp_idle_inhibit_manager_v1, z)
    WLF_GLOBAL_DESTROY(wp_text_input_manager_v3, z)
    WLF_GLOBAL_DESTROY(xdg_decoration_manager_v1, z)
    WLF_GLOBAL_DESTROY(xdg_output_manager_v1, z)

    if (context->wl_shm) {
        struct wlf_global *global = wl_shm_get_user_data(context->wl_shm);
        wlf_global_destroy(global);
#ifdef WL_SHM_RELEASE_SINCE_VERSION
        if (wl_shm_get_version(context->wl_shm) >= WL_SHM_RELEASE_SINCE_VERSION) {
            wl_shm_release(context->wl_shm);
        } else
#endif
        {
            wl_shm_destroy(context->wl_shm);
        }
        context->wl_shm = nullptr;
    }

    if (context->wp_pointer_gestures_v1) {
        struct wlf_global *global = zwp_pointer_gestures_v1_get_user_data(
            context->wp_pointer_gestures_v1);
        wlf_global_destroy(global);
        if (zwp_pointer_gestures_v1_get_version(context->wp_pointer_gestures_v1) >=
            ZWP_POINTER_GESTURES_V1_RELEASE_SINCE_VERSION)
        {
            zwp_pointer_gestures_v1_release(context->wp_pointer_gestures_v1);
        } else {
            zwp_pointer_gestures_v1_destroy(context->wp_pointer_gestures_v1);
        }
        context->wp_pointer_gestures_v1 = nullptr;
    }

#undef WLF_GLOBAL_DESTROY
}

static void *
wlf_global_bind(
    struct wlf_context *context,
    uint32_t name,
    const struct wl_interface *wl_interface,
    const void *listener,
    uint32_t version,
    uint32_t max_version)
{
    version = wlf_get_version(wl_interface, version, max_version);
    if (version == 0) {
        return nullptr;
    }

    struct wlf_global *global = wlf_global_create(context, name, version);
    if (!global) {
        return nullptr;
    }

    struct wl_proxy *proxy = wl_registry_bind(context->wl_registry, name, wl_interface, version);
    if (!proxy) {
        wlf_global_destroy(global);
        return nullptr;
    }

    if (listener && wl_interface->event_count > 0) {
        wl_proxy_add_listener(proxy, (void (**)(void))listener, global);
    } else {
        wl_proxy_set_user_data(proxy, global);
    }

    return proxy;
}

// region XDG WmBase

static void
xdg_wm_base_ping(void *, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

// endregion

// region WL Shm

static void
wl_shm_format(void *data, struct wl_shm *, uint32_t format)
{
    struct wlf_global *global = data;
    struct wlf_context *context = global->context;

    uint32_t *next = wl_array_add(&context->format_array, sizeof(uint32_t));
    if (!next) {
        return;
    }

    *next = format;
}

static const struct wl_shm_listener wl_shm_listener = {
    .format = wl_shm_format,
};

// endregion

// region WL Registry

static void
wl_registry_global(
    void *data,
    struct wl_registry *wl_registry,
    uint32_t name,
    const char *interface,
    uint32_t version)
{
    struct wlf_context *context = data;

#define WLF_MATCH(ident, prefix) \
    ((strcmp(interface, prefix##ident##_interface.name) == 0) && !context->ident)

    if WLF_MATCH(wl_compositor,) {
        context->wl_compositor = wlf_global_bind(
            context,
            name,
            &wl_compositor_interface,
            nullptr,
            version,
            WLF_WL_COMPOSITOR_VERSION);
    }
    else if WLF_MATCH(wl_subcompositor,) {
        context->wl_subcompositor = wlf_global_bind(
            context,
            name,
            &wl_subcompositor_interface,
            nullptr,
            version,
            WLF_WL_SUBCOMPOSITOR_VERSION);
    }
    else if WLF_MATCH(wl_shm,) {
        context->wl_shm = wlf_global_bind(
            context,
            name,
            &wl_shm_interface,
            &wl_shm_listener,
            version,
            WLF_WL_SHM_VERSION);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0) {
        struct wlf_seat *seat = wlf_seat_add(context, name, version);
        context->listener.seat(context->user_data, seat->global.id, true);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0) {
        wlf_output_bind(context, name, version);
    }
    else if WLF_MATCH(wl_data_device_manager,) {
        context->wl_data_device_manager = wlf_global_bind(
            context,
            name,
            &wl_data_device_manager_interface,
            nullptr,
            version,
            WLF_WL_DATA_DEVICE_MANAGER_VERSION);
    }
    else if WLF_MATCH(wp_viewporter,) {
        context->wp_viewporter = wlf_global_bind(
            context,
            name,
            &wp_viewporter_interface,
            nullptr,
            version,
            WLF_WP_VIEWPORTER_VERSION);
    }
    else if WLF_MATCH(wp_fractional_scale_manager_v1,) {
        context->wp_fractional_scale_manager_v1 = wlf_global_bind(
            context,
            name,
            &wp_fractional_scale_manager_v1_interface,
            nullptr,
            version,
            WLF_WP_FRACTIONAL_SCALE_MANAGER_V1_VERSION);
    }
    else if WLF_MATCH(wp_input_timestamps_manager_v1, z) {
        context->wp_input_timestamps_manager_v1 = wlf_global_bind(
            context,
            name,
            &zwp_input_timestamps_manager_v1_interface,
            nullptr,
            version,
            WLF_WP_INPUT_TIMESTAMPS_MANAGER_V1_VERSION);
    }
    else if WLF_MATCH(wp_text_input_manager_v3, z) {
        context->wp_text_input_manager_v3 = wlf_global_bind(
            context,
            name,
            &zwp_text_input_manager_v3_interface,
            nullptr,
            version,
            WLF_WP_TEXT_INPUT_MANAGER_V3_VERSION);
    }
    else if WLF_MATCH(wp_relative_pointer_manager_v1, z) {
        context->wp_relative_pointer_manager_v1 = wlf_global_bind(
            context,
            name,
            &zwp_relative_pointer_manager_v1_interface,
            nullptr,
            version,
            WLF_WP_RELATIVE_POINTER_MANAGER_V1_VERSION);
    }
    else if WLF_MATCH(wp_pointer_constraints_v1, z) {
        context->wp_pointer_constraints_v1 = wlf_global_bind(
            context,
            name,
            &zwp_pointer_constraints_v1_interface,
            nullptr,
            version,
            WLF_WP_POINTER_CONSTRAINTS_V1_VERSION);
    }
    else if WLF_MATCH(wp_pointer_gestures_v1, z) {
        context->wp_pointer_gestures_v1 = wlf_global_bind(
            context,
            name,
            &zwp_pointer_gestures_v1_interface,
            nullptr,
            version,
            WLF_WP_POINTER_GESTURES_V1_VERSION);
    }
    else if WLF_MATCH(wp_keyboard_shortcuts_inhibit_manager_v1, z) {
        context->wp_keyboard_shortcuts_inhibit_manager_v1 = wlf_global_bind(
            context,
            name,
            &zwp_keyboard_shortcuts_inhibit_manager_v1_interface,
            nullptr,
            version,
            WLF_WP_KEYBOARD_SHORTCUTS_INHIBIT_MANAGER_V1_VERSION);
    }
    else if WLF_MATCH(wp_idle_inhibit_manager_v1, z) {
        context->wp_idle_inhibit_manager_v1 = wlf_global_bind(
            context,
            name,
            &zwp_idle_inhibit_manager_v1_interface,
            nullptr,
            version,
            WLF_WP_IDLE_INHIBIT_MANAGER_V1_VERSION);
    }
    else if WLF_MATCH(wp_content_type_manager_v1,) {
        context->wp_content_type_manager_v1 = wlf_global_bind(
            context,
            name,
            &wp_content_type_manager_v1_interface,
            nullptr,
            version,
            WLF_WP_CONTENT_TYPE_MANAGER_V1_VERSION);
    }
    else if WLF_MATCH(wp_single_pixel_buffer_manager_v1,) {
        context->wp_single_pixel_buffer_manager_v1 = wlf_global_bind(
            context,
            name,
            &wp_single_pixel_buffer_manager_v1_interface,
            nullptr,
            version,
            WLF_WP_SINGLE_PIXEL_BUFFER_MANAGER_V1_VERSION);
    }
    else if WLF_MATCH(wp_alpha_modifier_v1,) {
        context->wp_alpha_modifier_v1 = wlf_global_bind(
            context,
            name,
            &wp_alpha_modifier_v1_interface,
            nullptr,
            version,
            WLF_WP_ALPHA_MODIFIER_V1_VERSION);
    }
    else if WLF_MATCH(xdg_wm_base,) {
        context->xdg_wm_base = wlf_global_bind(
            context,
            name,
            &xdg_wm_base_interface,
            &xdg_wm_base_listener,
            version,
            WLF_XDG_WM_BASE_VERSION);
    }
    else if WLF_MATCH(xdg_wm_dialog_v1,) {
        context->xdg_wm_dialog_v1 = wlf_global_bind(
            context,
            name,
            &xdg_wm_dialog_v1_interface,
            nullptr,
            version,
            WLF_XDG_WM_DIALOG_V1_VERSION);
    }
    else if WLF_MATCH(xdg_decoration_manager_v1, z) {
        context->xdg_decoration_manager_v1 = wlf_global_bind(
            context,
            name,
            &zxdg_decoration_manager_v1_interface,
            nullptr,
            version,
            WLF_XDG_DECORATION_MANAGER_V1_VERSION);
    }
    else if WLF_MATCH(xdg_output_manager_v1, z) {
        context->xdg_output_manager_v1 = wlf_global_bind(
            context,
            name,
            &zxdg_output_manager_v1_interface,
            nullptr,
            version,
            WLF_XDG_OUTPUT_MANAGER_V1_VERSION);
        if (context->xdg_output_manager_v1) {
            wlf_output_init_xdg_all(context);
        }
    }
    else if WLF_MATCH(ext_idle_notifier_v1,) {
        context->ext_idle_notifier_v1 = wlf_global_bind(
            context,
            name,
            &ext_idle_notifier_v1_interface,
            nullptr,
            version,
            WLF_EXT_IDLE_NOTIFICATION_V1_VERSION);
    }

#undef WLF_MATCH
}

static void
wl_registry_global_remove(
    void *data,
    struct wl_registry *wl_registry,
    uint32_t name)
{
    struct wlf_context *context = data;

    struct wlf_seat *seat, *seat_tmp;
    wl_list_for_each_safe(seat, seat_tmp, &context->seat_list, link) {
        if (seat->global.name == name) {
            context->listener.seat(context->user_data, seat->global.id, false);
            wlf_seat_remove(seat);
            return;
        }
    }

    struct wlf_output *output, *output_tmp;
    wl_list_for_each_safe(output, output_tmp, &context->output_list, link) {
        if (output->global.name == name) {
            wlf_output_destroy(output);
            return;
        }
    }

#define WLF_GLOBAL_REMOVE(ident, prefix) \
    if (context->ident) { \
        struct wlf_global *global = prefix##ident##_get_user_data(context->ident); \
        if (global->name == name) { \
            prefix##ident##_destroy(context->ident); \
            context->ident = nullptr;  \
            wlf_global_destroy(global); \
            return; \
        } \
    }

    WLF_GLOBAL_REMOVE(wl_data_device_manager,);
    WLF_GLOBAL_REMOVE(wp_viewporter,)
    WLF_GLOBAL_REMOVE(wp_fractional_scale_manager_v1,)
    WLF_GLOBAL_REMOVE(wp_content_type_manager_v1,)
    WLF_GLOBAL_REMOVE(wp_single_pixel_buffer_manager_v1,)
    WLF_GLOBAL_REMOVE(wp_alpha_modifier_v1,)
    WLF_GLOBAL_REMOVE(ext_idle_notifier_v1,)
    WLF_GLOBAL_REMOVE(xdg_wm_dialog_v1,)

    WLF_GLOBAL_REMOVE(wp_input_timestamps_manager_v1, z)
    WLF_GLOBAL_REMOVE(wp_relative_pointer_manager_v1, z)
    WLF_GLOBAL_REMOVE(wp_pointer_constraints_v1, z)
    WLF_GLOBAL_REMOVE(wp_keyboard_shortcuts_inhibit_manager_v1, z)
    WLF_GLOBAL_REMOVE(wp_idle_inhibit_manager_v1, z)
    WLF_GLOBAL_REMOVE(wp_text_input_manager_v3, z)
    WLF_GLOBAL_REMOVE(xdg_decoration_manager_v1, z)
    WLF_GLOBAL_REMOVE(xdg_output_manager_v1, z)

#undef WLF_GLOBAL_REMOVE

    if (context->wp_pointer_gestures_v1) {
        struct wlf_global *global = zwp_pointer_gestures_v1_get_user_data(
            context->wp_pointer_gestures_v1);
        if (global->name == name) {
            if (zwp_pointer_gestures_v1_get_version(context->wp_pointer_gestures_v1) >=
                ZWP_POINTER_GESTURES_V1_RELEASE_SINCE_VERSION)
            {
                zwp_pointer_gestures_v1_release(context->wp_pointer_gestures_v1);
            } else {
                zwp_pointer_gestures_v1_destroy(context->wp_pointer_gestures_v1);
            }
            context->wp_pointer_gestures_v1 = nullptr;
            wlf_global_destroy(global);
            return;
        }
    }

    // TODO: Remove temporary abort and implement proper context invalidation
    fprintf(stderr, "%s: Compositor removed critical global %u.\n", __func__, name);
    abort();
}

static const struct wl_registry_listener wl_registry_listener = {
    .global        = wl_registry_global,
    .global_remove = wl_registry_global_remove,
};

// endregion

static enum wlf_result
wlf_context_init(
    struct wlf_context *context,
    const struct wlf_context_info *info,
    const struct wlf_context_listener *listener)
{
    enum wlf_result result;
    context->listener = *listener;
    context->user_data = info->user_data;

    context->wl_display = wl_display_connect(info->display);
    if (!context->wl_display) {
        result = WLF_ERROR_WAYLAND;
        goto err_display;
    }

    wl_list_init(&context->seat_list);
    wl_list_init(&context->output_list);
    wl_list_init(&context->surface_list);
    wl_array_init(&context->format_array);

    context->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!context->xkb_context) {
        result = WLF_ERROR_UNKNOWN;
        goto err_xkb;
    }

    context->wl_registry = wl_display_get_registry(context->wl_display);
    if (!context->wl_registry) {
        result = WLF_ERROR_OUT_OF_MEMORY;
        goto err_registry;
    }

    wl_registry_add_listener(context->wl_registry, &wl_registry_listener, context);
    wl_display_roundtrip(context->wl_display);

    if (!context->wl_compositor ||
        !context->wl_subcompositor ||
        !context->wl_shm)
    {
        result = WLF_ERROR_WAYLAND;
        goto err_globals;
    }

    return WLF_SUCCESS;

err_globals:
    wlf_destroy_seats(context);
    wlf_destroy_outputs(context);
    wlf_destroy_globals(context);
    wl_registry_destroy(context->wl_registry);
    wl_display_roundtrip(context->wl_display);
err_registry:
    xkb_context_unref(context->xkb_context);
err_xkb:
    wl_array_release(&context->format_array);
    wl_display_disconnect(context->wl_display);
err_display:
    return result;
}

static void
wlf_context_fini(struct wlf_context *context)
{
    wlf_destroy_seats(context);
    wlf_destroy_outputs(context);
    wlf_destroy_globals(context);
    wl_registry_destroy(context->wl_registry);
    wl_display_roundtrip(context->wl_display);
    xkb_context_unref(context->xkb_context);
    wl_array_release(&context->format_array);
    wl_display_disconnect(context->wl_display);
}

// region Public API

enum wlf_result
wlf_context_create(
    const struct wlf_context_info *info,
    const struct wlf_context_listener *listener,
    struct wlf_context **_context)
{
    struct wlf_context *context = calloc(1, sizeof(*context));
    if (!context) {
        return WLF_ERROR_OUT_OF_MEMORY;
    }

    enum wlf_result result = wlf_context_init(context, info, listener);
    if (result < WLF_SUCCESS) {
        free(context);
        return result;
    }

    *_context = context;
    return result;
}

void
wlf_context_destroy(struct wlf_context *context)
{
    wlf_context_fini(context);
    free(context);
}

enum wlf_result
wlf_dispatch_events(struct wlf_context *context, int64_t timeout)
{
    struct wl_display *wl_display = context->wl_display;

    if (wl_display_prepare_read(wl_display) < 0) {
        int n = wl_display_dispatch_pending(wl_display);
        return n < 0 ? WLF_ERROR_WAYLAND : WLF_SUCCESS;
    }

    int n = wlf_flush(wl_display);
    if (n < 0) {
        wl_display_cancel_read(wl_display);
        return WLF_ERROR_WAYLAND;
    }

    struct pollfd fds[1];
    fds[0].fd = wl_display_get_fd(wl_display);
    fds[0].events = POLLIN;

    do {
        n = poll(fds, 1, 0);
    } while (n < 0 && errno == EINTR);

    if (n < 0) {
        wl_display_cancel_read(wl_display);
        return WLF_ERROR_WAYLAND;
    }

    if (fds[0].revents & POLLIN) {
        n = wl_display_read_events(wl_display);
        if (n < 0) {
            return WLF_ERROR_WAYLAND;
        }
    } else {
        wl_display_cancel_read(wl_display);
    }

    n = wl_display_dispatch_pending(wl_display);
    if (n < 0) {
        return WLF_ERROR_WAYLAND;
    }

    return WLF_SUCCESS;
}

// endregion
