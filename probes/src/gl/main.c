// probe_gl v3 -- minimal & robust.
// v1 crashed during interactive paging; v2 crashed before writing the SD report
// (so the fault is in the GL-gather / fs-mount path, not in teardown).
// v3: no SD card, no paging, no full extension dump -- just ONE screen of the
// facts we actually need. ASCII only. Run in APPLICATION mode. Press + to exit.
#include <stdio.h>
#include <string.h>
#include <switch.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#if defined(__has_include)
#  if __has_include(<GLES3/gl3.h>)
#    include <GLES3/gl3.h>
#  endif
#endif

typedef struct { const char *name; const char *why; } ext_t;

static const ext_t k_ext[] = {
    { "GL_OES_standard_derivatives",       "Skia gradients/AA" },
    { "GL_OES_element_index_uint",         "32-bit indices" },
    { "GL_OES_vertex_array_object",        "VAO" },
    { "GL_OES_EGL_image",                  "EGLImage interop" },
    { "GL_OES_EGL_image_external",         "external textures" },
    { "GL_OES_mapbuffer",                  "buffer mapping" },
    { "GL_OES_rgb8_rgba8",                 "RGBA8 targets" },
    { "GL_OES_depth24",                    "depth24" },
    { "GL_OES_packed_depth_stencil",       "packed depth/stencil" },
    { "GL_OES_texture_npot",               "NPOT textures" },
    { "GL_OES_fbo_render_mipmap",          "render to mipmap" },
    { "GL_EXT_texture_format_BGRA8888",    "BGRA textures" },
    { "GL_EXT_texture_storage",            "immutable textures" },
    { "GL_EXT_sRGB",                       "sRGB" },
    { "GL_EXT_sRGB_write_control",         "sRGB write ctl" },
    { "GL_EXT_frag_depth",                 "frag depth" },
    { "GL_EXT_shader_texture_lod",         "explicit LOD" },
    { "GL_EXT_discard_framebuffer",        "discard fb" },
    { "GL_EXT_occlusion_query_boolean",    "occlusion query" },
    { "GL_EXT_robustness",                 "robustness" },
    { "GL_EXT_multisampled_render_to_texture", "MSAA" },
    { "GL_EXT_disjoint_timer_query",       "GPU timing" },
    { "GL_KHR_debug",                      "debug output" },
    { "GL_EXT_texture_compression_s3tc",   "S3TC" },
    { "GL_EXT_texture_filter_anisotropic", "aniso filter" },
    { "GL_EXT_buffer_storage",             "immutable buffers" },
};
#define N_EXT (sizeof(k_ext)/sizeof(k_ext[0]))

// Values captured during the GL phase, printed after the context is gone.
static char gl_ver[256], gl_vend[128], gl_rend[128], gl_glsl[256];
static char egl_ver[128], egl_vend[128];
static int  g_es = 0, g_samples = 0, g_total_ext = 0, g_missing = 0;
static int  g_missing_idx[64];
static int  g_tex = 0, g_rb = 0, g_attr = 0, g_vary = 0;
static int  g_stage = 0;        // last completed stage, for diagnosis

static const char *g_ext = NULL;

static int ext_present(const char *name)
{
    if (!g_ext) return 0;
    size_t n = strlen(name);
    const char *p = g_ext;
    while ((p = strstr(p, name)) != NULL) {
        char after = p[n];
        if (after == ' ' || after == '\0') return 1;
        p += n;
    }
    return 0;
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    consoleInit(NULL);
    printf("probe_gl v3 starting (no SD, no paging)...\n");
    printf("Run in APPLICATION mode. One screen of results follows.\n");
    consoleUpdate(NULL);
    svcSleepThread(2000000000ull);
    consoleExit(NULL);

    EGLDisplay dpy = EGL_NO_DISPLAY;
    EGLSurface surf = EGL_NO_SURFACE;
    EGLContext ctx = EGL_NO_CONTEXT;
    EGLConfig cfg = NULL;
    EGLint ncfg = 0;
    g_stage = 1;

    eglBindAPI(EGL_OPENGL_ES_API);
    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY) { g_stage = -1; goto out; }
    {
        EGLint mj = 0, mn = 0;
        if (!eglInitialize(dpy, &mj, &mn)) { g_stage = -2; goto out; }
        snprintf(egl_ver, sizeof(egl_ver), "%d.%d", mj, mn);
        snprintf(egl_vend, sizeof(egl_vend), "%s", eglQueryString(dpy, EGL_VENDOR));
    }
    g_stage = 2;
    {
        const EGLint cfg_attr[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8, EGL_NONE
        };
        if (!eglChooseConfig(dpy, cfg_attr, &cfg, 1, &ncfg) || ncfg < 1) { g_stage = -3; goto out; }
        eglGetConfigAttrib(dpy, cfg, EGL_SAMPLES, &g_samples);
    }
    g_stage = 3;
    surf = eglCreateWindowSurface(dpy, cfg, nwindowGetDefault(), NULL);
    if (surf == EGL_NO_SURFACE) { g_stage = -4; goto out; }
    g_stage = 4;
    {
        const EGLint c3[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, c3);
        if (ctx != EGL_NO_CONTEXT) g_es = 3;
    }
    if (ctx == EGL_NO_CONTEXT) {
        const EGLint c2[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
        ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, c2);
        if (ctx != EGL_NO_CONTEXT) g_es = 2;
    }
    if (ctx == EGL_NO_CONTEXT) { g_stage = -5; goto out; }
    g_stage = 5;
    if (!eglMakeCurrent(dpy, surf, surf, ctx)) { g_stage = -6; goto out; }
    eglSwapInterval(dpy, 1);
    g_stage = 6;

    snprintf(gl_ver,  sizeof(gl_ver),  "%s", (const char *)glGetString(GL_VERSION));
    snprintf(gl_vend, sizeof(gl_vend), "%s", (const char *)glGetString(GL_VENDOR));
    snprintf(gl_rend, sizeof(gl_rend), "%s", (const char *)glGetString(GL_RENDERER));
    snprintf(gl_glsl, sizeof(gl_glsl), "%s", (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
    g_stage = 7;

    {
        GLint v = -1;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &v);     g_tex  = v;
        glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &v); g_rb  = v;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &v);    g_attr = v;
        glGetIntegerv(GL_MAX_VARYING_VECTORS, &v);   g_vary = v;
    }
    g_stage = 8;

    g_ext = (const char *)glGetString(GL_EXTENSIONS);
    g_stage = 9;

    // count + key-extension check (no glGetStringi: not needed, and it is a
    // known crash risk on stub GLES implementations)
    if (g_ext) {
        const char *p = g_ext;
        while (*p) { while (*p == ' ') p++; if (!*p) break; while (*p && *p != ' ') p++; g_total_ext++; }
    }
    for (unsigned i = 0; i < N_EXT; i++) {
        if (!ext_present(k_ext[i].name)) {
            if (g_missing < 64) g_missing_idx[g_missing] = (int)i;
            g_missing++;
        }
    }
    g_stage = 10;

out:
    if (dpy != EGL_NO_DISPLAY) {
        eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (ctx != EGL_NO_CONTEXT) eglDestroyContext(dpy, ctx);
        if (surf != EGL_NO_SURFACE) eglDestroySurface(dpy, surf);
        eglTerminate(dpy);
    }
    g_stage = (g_stage == 10) ? 11 : g_stage;

    consoleInit(NULL);
    printf("=== probe_gl v3 summary ===\n");
    if (g_stage < 0 || (g_stage >= 1 && g_stage < 11 && g_stage != 11))
        printf("!! aborted at stage %d (negative = failure point)\n", g_stage);
    printf("EGL %s  vendor=%s\n", egl_ver[0] ? egl_ver : "?", egl_vend[0] ? egl_vend : "?");
    printf("GL  %s\n", gl_ver[0] ? gl_ver : "?");
    printf("VENDOR %s\n", gl_vend[0] ? gl_vend : "?");
    printf("RENDER %s\n", gl_rend[0] ? gl_rend : "?");
    printf("GLSL %s\n", gl_glsl[0] ? gl_glsl : "?");
    printf("ctx ES%d  samples=%d\n", g_es, g_samples);
    printf("limits tex=%d rb=%d attribs=%d vary=%d\n", g_tex, g_rb, g_attr, g_vary);
    printf("extensions=%d   missing %d of %d\n", g_total_ext, g_missing, (int)N_EXT);
    printf("-- missing --\n");
    for (int k = 0; k < g_missing && k < 64; k++) {
        const ext_t *e = &k_ext[g_missing_idx[k]];
        printf("%s  (%s)\n", e->name, e->why);
    }
    if (g_missing == 0) printf("(none)\n");
    printf("\nPress + to exit\n");
    consoleUpdate(NULL);

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);
    while (appletMainLoop()) {
        padUpdate(&pad);
        if (padGetButtonsDown(&pad) & HidNpadButton_Plus) break;
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
    return 0;
}
