#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <EGL/egl.h>
#include <GL/gl.h>

#include <wlf/context.h>
#include <wlf/input.h>
#include <wlf/surface.h>
#include <wlf/toplevel.h>
#include <wlf/popup.h>
#include <wlf/egl.h>

#define array_len(a) (sizeof(a) / sizeof(a[0]))

static const GLfloat TAU_F = 6.28318530717958647692f;
static GLfloat g_angle = 0.0f;

static const EGLint g_config_attribs[] = {
    EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
    EGL_RED_SIZE,        8,
    EGL_GREEN_SIZE,      8,
    EGL_BLUE_SIZE,       8,
    EGL_ALPHA_SIZE,      8,
    EGL_DEPTH_SIZE,      24,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
    EGL_NONE
};

static const EGLint g_context_attribs[] = {
    EGL_CONTEXT_MAJOR_VERSION, 2,
    EGL_CONTEXT_MINOR_VERSION, 0,
    EGL_NONE
};

struct renderer {
    EGLSurface surface;
    EGLContext context;
    GLfloat view_rot[3];
    GLuint gear[3];
    GLsizei width;
    GLsizei height;
    bool resized;
};

struct window {
    struct demo *demo;
    struct wlf_toplevel *toplevel;
    struct renderer r;
    bool closed;
    bool initialized;
};

struct demo {
    struct wlf_context *context;
    struct wlf_seat *seat;
    uint64_t seat_id;

    EGLDisplay display;
    EGLConfig config;
};

static int64_t
get_time_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000 + (int64_t)ts.tv_nsec;
}

static inline GLfloat
calc_angle(GLint i, GLint teeth)
{
    return (GLfloat)i * TAU_F / (GLfloat)teeth;
}

static void
gear(GLfloat inner_radius,
     GLfloat outer_radius,
     GLfloat width,
     GLint teeth,
     GLfloat tooth_depth)
{
    GLfloat r0, r1, r2;
    GLfloat angle, da;
    GLfloat u, v, len;

    r0 = inner_radius;
    r1 = outer_radius - tooth_depth / 2.0f;
    r2 = outer_radius + tooth_depth / 2.0f;

    da = TAU_F / (GLfloat)teeth / 4.0f;

    glShadeModel(GL_FLAT);
    glNormal3f(0.0f, 0.0f, 1.0f);

    glBegin(GL_QUAD_STRIP);
    for (GLint i = 0; i <= teeth; i++) {
        angle = calc_angle(i, teeth);

        glVertex3f(r0 * cosf(angle), r0 * sinf(angle), width * 0.5f);
        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), width * 0.5f);
        if (i < teeth) {
            glVertex3f(r0 * cosf(angle), r0 * sinf(angle), width * 0.5f);
            glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), width * 0.5f);
        }
    }
    glEnd();

    glBegin(GL_QUADS);
    for (GLint i = 0; i < teeth; i++) {
        angle = calc_angle(i, teeth);

        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), width * 0.5f);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), width * 0.5f);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), width * 0.5f);
        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), width * 0.5f);
    }
    glEnd();

    glNormal3f(0.0f, 0.0f, -1.0f);

    glBegin(GL_QUAD_STRIP);
    for (GLint i = 0; i <= teeth; i++) {
        angle = calc_angle(i, teeth);

        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), -width * 0.5f);
        glVertex3f(r0 * cosf(angle), r0 * sinf(angle), -width * 0.5f);
        if (i < teeth) {
            glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), -width * 0.5f);
            glVertex3f(r0 * cosf(angle), r0 * sinf(angle), -width * 0.5f);
        }
    }
    glEnd();

    glBegin(GL_QUADS);
    for (GLint i = 0; i < teeth; i++) {
        angle = calc_angle(i, teeth);

        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), -width * 0.5f);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), -width * 0.5f);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), -width * 0.5f);
        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), -width * 0.5f);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    for (GLint i = 0; i < teeth; i++) {
        angle = calc_angle(i, teeth);

        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), width * 0.5f);
        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), -width * 0.5f);
        u = r2 * cosf(angle + da) - r1 * cosf(angle);
        v = r2 * sinf(angle + da) - r1 * sinf(angle);
        len = sqrtf(u * u + v * v);
        u /= len;
        v /= len;
        glNormal3f(v, -u, 0.0f);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), width * 0.5f);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), -width * 0.5f);
        glNormal3f(cosf(angle), sinf(angle), 0.0f);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), width * 0.5f);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), -width * 0.5f);
        u = r1 * cosf(angle + 3.0f * da) - r2 * cosf(angle + 2.0f * da);
        v = r1 * sinf(angle + 3.0f * da) - r2 * sinf(angle + 2.0f * da);
        glNormal3f(v, -u, 0.0f);
        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), width * 0.5f);
        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), -width * 0.5f);
        glNormal3f(cosf(angle), sinf(angle), 0.0f);
    }

    glVertex3f(r1 * cosf(0), r1 * sinf(0), width * 0.5f);
    glVertex3f(r1 * cosf(0), r1 * sinf(0), -width * 0.5f);

    glEnd();

    glShadeModel(GL_SMOOTH);

    glBegin(GL_QUAD_STRIP);
    for (GLint i = 0; i <= teeth; i++) {
        angle = calc_angle(i, teeth);
        glNormal3f(-cosf(angle), -sinf(angle), 0.0f);
        glVertex3f(r0 * cosf(angle), r0 * sinf(angle), -width * 0.5f);
        glVertex3f(r0 * cosf(angle), r0 * sinf(angle), width * 0.5f);
    }
    glEnd();
}

// region Renderer

static void
renderer_draw(struct renderer *r)
{
    if (r->resized) {
        glViewport(0, 0, r->width, r->height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        GLfloat hf = (GLfloat) r->height;
        GLfloat wf = (GLfloat) r->width;

        if (hf > wf) {
            GLfloat aspect = hf / wf;
            glFrustum(-1.0, 1.0, -aspect, aspect, 5.0, 60.0);
        } else {
            GLfloat aspect = wf / hf;
            glFrustum(-aspect, aspect, -1.0, 1.0, 5.0, 60.0);
        }

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -40.0f);
        r->resized = false;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.8f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glRotatef(r->view_rot[0], 1.0f, 0.0f, 0.0f);
    glRotatef(r->view_rot[1], 0.0f, 1.0f, 0.0f);
    glRotatef(r->view_rot[2], 0.0f, 0.0f, 1.0f);

    glPushMatrix();
    glTranslatef(-3.0f, -2.0f, 0.0f);
    glRotatef(g_angle, 0.0f, 0.0f, 1.0f);
    glCallList(r->gear[0]);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(3.1f, -2.0f, 0.0f);
    glRotatef(-2.0f * g_angle - 9.0f, 0.0f, 0.0f, 1.0f);
    glCallList(r->gear[1]);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-3.1f, 4.2f, 0.0f);
    glRotatef(-2.0f * g_angle - 25.0f, 0.0f, 0.0f, 1.0f);
    glCallList(r->gear[2]);
    glPopMatrix();

    glPopMatrix();
}

static void
renderer_init_gears(struct renderer *r)
{
    static GLfloat pos[4] = { 5.0f, 5.0f, 10.0f, 0.0f };
    static GLfloat red[4] = { 0.8f, 0.1f, 0.0f, 1.0f };
    static GLfloat green[4] = { 0.0f, 0.8f, 0.2f, 1.0f };
    static GLfloat blue[4] = { 0.2f, 0.2f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);

    r->gear[0] = glGenLists(1);
    glNewList(r->gear[0], GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
    gear(1.0f, 4.0f, 1.0f, 20, 0.7f);
    glEndList();

    r->gear[1] = glGenLists(1);
    glNewList(r->gear[1], GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
    gear(0.5f, 2.0f, 2.0f, 10, 0.7f);
    glEndList();

    r->gear[2] = glGenLists(1);
    glNewList(r->gear[2], GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
    gear(1.3f, 2.0f, 0.5f, 10, 0.7f);
    glEndList();

    glEnable(GL_NORMALIZE);
}

static void
renderer_init_surface(struct demo *demo, struct wlf_surface *s, struct renderer *r)
{
    r->context = eglCreateContext(
        demo->display,
        demo->config,
        EGL_NO_CONTEXT,
        g_context_attribs);
    if (r->context == EGL_NO_CONTEXT) {
        fprintf(stderr, "eglCreateContext failed: 0x%08x\n", eglGetError());
        abort();
    }

    r->surface = wlfCreateEGLSurface(
        eglCreatePlatformWindowSurface,
        demo->display,
        s,
        demo->config);
    if (r->surface == EGL_NO_SURFACE) {
        fprintf(stderr, "eglCreateWindowSurface failed: %d", eglGetError());
        abort();
    }
}

static void
renderer_make_current(struct demo *demo, struct renderer *r)
{
    EGLBoolean ok = eglMakeCurrent(demo->display, r->surface, r->surface, r->context);
    if (ok == EGL_FALSE) {
        fprintf(stderr, "eglMakeCurrent failed: 0x%08x\n", eglGetError());
        abort();
    }
}

static void
renderer_present(struct demo *demo, struct renderer *r)
{
    EGLBoolean ok = eglSwapBuffers(demo->display, r->surface);
    if (ok == EGL_FALSE) {
        printf("eglSwapBuffers failed: %08x\n", eglGetError());
        abort();
    }
}

// endregion

// region Window

static void
window_close(void *user_data)
{
    struct window *win = user_data;
    win->closed = true;
}

static void
window_configure(void *user_data, struct wlf_extent extent)
{
    struct window *win = user_data;
    if (!win->initialized) {
        struct wlf_surface *s = wlf_toplevel_get_surface(win->toplevel);

        renderer_init_surface(win->demo, s, &win->r);
        renderer_make_current(win->demo, &win->r);
        // eglSwapInterval(demo->display, -1);
        renderer_init_gears(&win->r);

        win->r.view_rot[0] = 20.0f;
        win->r.view_rot[1] = 30.0f;
        win->r.view_rot[2] = 0.0f;

        win->initialized = true;
    }
    win->r.width = extent.width;
    win->r.height = extent.height;
    win->r.resized = true;
}

static const struct wlf_toplevel_listener toplevel_listener = {
    .close     = window_close,
    .configure = window_configure,
};

static struct window *
window_create(struct demo *demo)
{
    struct window *window = calloc(1, sizeof(*window));
    if (!window) {
        return nullptr;
    }
    window->demo = demo;

    const char8_t title[] = u8"eglGears";
    const char8_t app_id[] = u8"wleaf.eglgears";

    struct wlf_toplevel_info info = {
        .flags           = WLF_TOPLEVEL_FLAGS_NONE,
        .extent          = { 640, 360 },
        .decoration_mode = WLF_DECORATION_MODE_SERVER_SIDE,
        .content_type    = WLF_CONTENT_TYPE_NONE,
        .parent          = nullptr,
        .title           = title,
        .app_id          = app_id,
        .user_data       = window,
    };

    enum wlf_result res = wlf_toplevel_create(
        demo->context,
        &info,
        &toplevel_listener,
        &window->toplevel);
    if (res >= WLF_SUCCESS) {
        return window;
    }

    free(window);
    return nullptr;
}

static void
window_destroy(struct window *win)
{
    struct demo *demo = win->demo;
    if (win->r.surface) {
        eglDestroySurface(demo->display, win->r.surface);
    }
    if (win->r.context) {
        eglDestroyContext(demo->display, win->r.context);
    }
    wlf_toplevel_destroy(win->toplevel);
    free(win);
}

// endregion

// region Demo

static void
seat_name(void *data, const char8_t *name)
{
}

static void
seat_idled(void *data, bool idle)
{
}

static void
seat_shortcuts_inhibited(void *data, struct wlf_surface *surface, bool inhibited)
{
}

static const struct wlf_seat_listener seat_listener = {
    .name = seat_name,
    .idled = seat_idled,
    .shortcuts_inhibited = seat_shortcuts_inhibited,
};

static void
on_seat(void *data, uint64_t id, bool added)
{
    struct demo *demo = data;
    if (added) {
        if (demo->seat_id == 0) {
            demo->seat_id = id;
        }
    } else if (demo->seat) {
        wlf_seat_release(demo->seat);
        demo->seat = nullptr;
        demo->seat_id = 0;
    }
}

static const struct wlf_context_listener context_listener = {
    .seat = on_seat,
};

static int
demo_init(struct demo *demo)
{
    struct wlf_context_info info = {
        .flags     = WLF_CONTEXT_FLAGS_NONE,
        .display   = nullptr,
        .user_data = demo,
    };
    enum wlf_result res = wlf_context_create(&info, &context_listener, &demo->context);
    if (res != WLF_SUCCESS) {
        fprintf(stderr, "wlf_context_create failed: %d\n", res);
        return -1;
    }

    if (demo->seat_id != 0) {
        struct wlf_seat_info seat_info = {
            .id = demo->seat_id,
            .user_data = demo,
        };
        res = wlf_seat_bind(demo->context, &seat_info, &seat_listener, &demo->seat);
        if (res < WLF_SUCCESS) {
            fprintf(stderr, "wlf_seat_bind failed: %d\n", res);
            return -1;
        }
    }

    demo->display = wlfGetEGLDisplay(eglGetPlatformDisplay, demo->context);
    if (demo->display == EGL_NO_DISPLAY) {
        fprintf(stderr, "eglGetPlatformDisplay failed: %d\n", eglGetError());
        goto err1;
    }

    EGLint major, minor;
    EGLBoolean ok = eglInitialize(demo->display, &major, &minor);
    if (ok == EGL_FALSE) {
        fprintf(stderr, "eglInitialize failed: 0x%08x\n", eglGetError());
        goto err1;
    }

    if (major < 1 || (major == 1 && minor < 4)) {
        fprintf(stderr, "EGL version %d.%d is too old\n", major, minor);
        goto err2;
    }

    ok = eglBindAPI(EGL_OPENGL_API);
    if (ok == EGL_FALSE) {
        fprintf(stderr, "eglBindAPI failed: 0x%08x\n", eglGetError());
        goto err2;
    }

    EGLint num_configs = 1;
    ok = eglChooseConfig(demo->display, g_config_attribs, &demo->config, 1, &num_configs);
    if (ok == EGL_FALSE) {
        fprintf(stderr, "eglChooseConfig failed: 0x%08x\n", eglGetError());
        goto err2;
    }

    if (num_configs == 0) {
        fprintf(stderr, "eglChooseConfig failed: no configs found\n");
        goto err2;
    }

    return 0;
err2:
    eglTerminate(demo->display);
err1:
    wlf_context_destroy(demo->context);
    return -1;
}

static void
demo_fini(struct demo *demo)
{
    eglTerminate(demo->display);
    wlf_context_destroy(demo->context);
}

// endregion

int
main(int argc, char *argv[])
{
    struct demo demo = {};
    demo_init(&demo);

    struct window *win = window_create(&demo);

    int64_t last_time = get_time_ns();

    while (!win->closed) {
        enum wlf_result res = wlf_dispatch_events(demo.context, 0);
        if (res != WLF_SUCCESS) {
            break;
        }

        if (win->initialized) {
            renderer_make_current(&demo, &win->r);
            renderer_draw(&win->r);
            renderer_present(&demo, &win->r);
        }

        int64_t now = get_time_ns();
        int64_t dt = now - last_time;
        last_time = now;

        float time = (float)dt / 1000000000.0f;
        g_angle += time * 70.0f;
        g_angle = fmodf(g_angle, 360.0f);
    }

    window_destroy(win);
    demo_fini(&demo);
    return 0;
}
