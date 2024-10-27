#pragma once

EGLDisplay
wlfGetEGLDisplay(
    PFNEGLGETPLATFORMDISPLAYPROC proc,
    struct wlf_context *context);

EGLSurface
wlfCreateEGLSurface(
    PFNEGLCREATEPLATFORMWINDOWSURFACEPROC proc,
    EGLDisplay display,
    struct wlf_surface *surface,
    EGLConfig config);

void
wlfDestroyEGLSurface(
    PFNEGLDESTROYSURFACEPROC proc,
    EGLDisplay display,
    struct wlf_surface *surface,
    EGLSurface egl_surface);
