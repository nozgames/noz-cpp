//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//
//  Web/Emscripten WebGL renderer implementation
//

#include "gl_internal.h"

#include <emscripten.h>
#include <emscripten/html5.h>

// Shared functions from gl_render.cpp
extern void CreateOffscreenTarget(OffscreenTarget& target, int width, int height, int samples);
extern void DestroyOffscreenTarget(OffscreenTarget& target);

// Web-specific state
static struct {
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
    const char* canvas_id;
    bool context_lost;
} g_webgl = {};

// Context lost/restored callbacks
static EM_BOOL OnContextLost(int event_type, const void* reserved, void* user_data) {
    (void)event_type;
    (void)reserved;
    (void)user_data;
    LogError("WebGL context lost!");
    g_webgl.context_lost = true;
    return EM_TRUE;
}

static EM_BOOL OnContextRestored(int event_type, const void* reserved, void* user_data) {
    (void)event_type;
    (void)reserved;
    (void)user_data;
    LogInfo("WebGL context restored");
    g_webgl.context_lost = false;
    // Note: Would need to recreate all GL resources here
    return EM_TRUE;
}

bool IsWebGLContextValid() {
    return g_webgl.context > 0 && !g_webgl.context_lost;
}

void InitWebGL(const RendererTraits* traits, const char* canvas_id) {
    memset(&g_gl, 0, sizeof(g_gl));
    memset(&g_webgl, 0, sizeof(g_webgl));
    g_gl.traits = *traits;
    g_webgl.canvas_id = canvas_id;
    g_gl.depth_conversion_factor = 1.0f / (traits->max_depth - traits->min_depth);

    // Create WebGL2 context
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    attrs.alpha = EM_FALSE;
    attrs.depth = EM_TRUE;
    attrs.stencil = EM_TRUE;
    attrs.antialias = traits->msaa_samples > 0 ? EM_TRUE : EM_FALSE;
    attrs.premultipliedAlpha = EM_FALSE;
    attrs.preserveDrawingBuffer = EM_FALSE;
    attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
    attrs.failIfMajorPerformanceCaveat = EM_FALSE;
    attrs.enableExtensionsByDefault = EM_TRUE;

    g_webgl.context = emscripten_webgl_create_context(canvas_id, &attrs);
    if (g_webgl.context <= 0) {
        LogError("Failed to create WebGL2 context, error: %d", g_webgl.context);
        // Try WebGL1 as fallback
        attrs.majorVersion = 1;
        g_webgl.context = emscripten_webgl_create_context(canvas_id, &attrs);
        if (g_webgl.context <= 0) {
            Exit("Failed to create WebGL context");
            return;
        }
        LogInfo("Using WebGL 1.0 (WebGL2 not available)");
    }

    emscripten_webgl_make_context_current(g_webgl.context);

    // Register context lost/restored handlers
    emscripten_set_webglcontextlost_callback(canvas_id, nullptr, EM_TRUE, OnContextLost);
    emscripten_set_webglcontextrestored_callback(canvas_id, nullptr, EM_TRUE, OnContextRestored);

    // Get initial canvas size
    int width, height;
    emscripten_webgl_get_drawing_buffer_size(g_webgl.context, &width, &height);
    g_gl.screen_size = {width, height};
    g_gl.native_screen_size = {width, height};

    // Create default VAO (required in WebGL2)
    glGenVertexArrays(1, &g_gl.current_vao);
    glBindVertexArray(g_gl.current_vao);

    // Set up default GL state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // WebGL uses lower-left origin and -1 to 1 depth by default
    // We need to handle this in shaders since glClipControl isn't available
    glFrontFace(GL_CCW);

    // Set initial viewport
    glViewport(0, 0, g_gl.screen_size.x, g_gl.screen_size.y);

    // Create Uniform Buffer Objects
    glGenBuffers(UNIFORM_BUFFER_COUNT, g_gl.ubos);
    for (int i = 0; i < UNIFORM_BUFFER_COUNT; i++) {
        glBindBuffer(GL_UNIFORM_BUFFER, g_gl.ubos[i]);
        glBufferData(GL_UNIFORM_BUFFER, MAX_UNIFORM_BUFFER_SIZE, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, i, g_gl.ubos[i]);
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Determine MSAA sample count
    int samples = 1;
    if (traits->msaa_samples > 1) {
        GLint max_samples = 0;
        glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
        samples = traits->msaa_samples;
        if (samples > max_samples) {
            samples = max_samples;
        }
        if (samples < 2) {
            samples = 1;
        }
    }

    // Create offscreen render target (scene + UI combined)
    CreateOffscreenTarget(g_gl.offscreen, g_gl.screen_size.x, g_gl.screen_size.y, samples);
}

void ResizeWebGL(const Vec2Int& size) {
    if (size.x <= 0 || size.y <= 0)
        return;

    g_gl.native_screen_size = size;
}

void PlatformSetRenderSize(Vec2Int logical_size, Vec2Int native_size) {
    if (logical_size.x <= 0 || logical_size.y <= 0)
        return;

    // Skip if context is lost
    if (g_webgl.context_lost) {
        LogError("PlatformSetRenderSize: context lost, skipping");
        return;
    }

    bool logical_changed = g_gl.screen_size != logical_size;

    g_gl.native_screen_size = native_size;

    if (logical_changed) {
        emscripten_webgl_make_context_current(g_webgl.context);
        glFinish();

        g_gl.screen_size = logical_size;
        int samples = g_gl.offscreen.samples;
        CreateOffscreenTarget(g_gl.offscreen, logical_size.x, logical_size.y, samples);
    }
}

void ShutdownWebGL() {
    // Delete offscreen target
    DestroyOffscreenTarget(g_gl.offscreen);

    // Delete UBOs
    glDeleteBuffers(UNIFORM_BUFFER_COUNT, g_gl.ubos);

    if (g_gl.current_vao) {
        glDeleteVertexArrays(1, &g_gl.current_vao);
        g_gl.current_vao = 0;
    }

    if (g_webgl.context > 0) {
        emscripten_webgl_destroy_context(g_webgl.context);
        g_webgl.context = 0;
    }
}

void PlatformEndRender() {
    // WebGL handles buffer swapping automatically
}

// Entry points for the platform abstraction
void InitRenderDriver(const RendererTraits* traits, const char* canvas_id) {
    InitWebGL(traits, canvas_id);
}

void ResizeRenderDriver(const Vec2Int& size) {
    ResizeWebGL(size);
}

void ShutdownRenderDriver() {
    ShutdownWebGL();
}
