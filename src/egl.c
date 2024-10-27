#include <assert.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-egl-core.h>

#define EGL_EGL_PROTOTYPES 0
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "context_priv.h"
#include "surface_priv.h"

#include "wlf/egl.h"

EGLDisplay
wlfGetEGLDisplay(
    PFNEGLGETPLATFORMDISPLAYPROC proc,
    struct wlf_context *context)
{
    return proc(EGL_PLATFORM_WAYLAND_KHR, context->wl_display, nullptr);
}

EGLSurface
wlfCreateEGLSurface(
    PFNEGLCREATEPLATFORMWINDOWSURFACEPROC proc,
    EGLDisplay display,
    struct wlf_surface *surface,
    EGLConfig config)
{
    struct wl_egl_window *win = wlf_surface_create_egl_window(surface);
    if (!win) {
        return nullptr;
    }

    EGLSurface egl = proc(display, config, win, nullptr);
    if (surface == EGL_NO_SURFACE) {
        wlf_surface_destroy_egl_window(surface);
        return nullptr;
    }

    return egl;
}

void
wlfDestroyEGLSurface(
    PFNEGLDESTROYSURFACEPROC proc,
    EGLDisplay display,
    struct wlf_surface *surface,
    EGLSurface egl_surface)
{
    assert(surface->wl_egl_window);
    proc(display, egl_surface);
    wl_egl_window_destroy(surface->wl_egl_window);
    surface->wl_egl_window = nullptr;
}
