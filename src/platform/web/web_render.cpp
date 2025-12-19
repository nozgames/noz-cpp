//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
//  Web/Emscripten WebGL renderer implementation
//

#include "../gl/gl_internal.h"

#include <emscripten.h>
#include <emscripten/html5.h>

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

// Note: The gles_render.cpp provides most of the rendering implementation.
// This file provides the WebGL-specific initialization and frame management.

static void CreateOffscreenTarget(OffscreenTarget& target, int width, int height, int samples) {
    // Delete existing resources
    if (target.framebuffer) {
        glDeleteFramebuffers(1, &target.framebuffer);
        target.framebuffer = 0;
    }
    if (target.texture) {
        glDeleteTextures(1, &target.texture);
        target.texture = 0;
    }
    if (target.depth_renderbuffer) {
        glDeleteRenderbuffers(1, &target.depth_renderbuffer);
        target.depth_renderbuffer = 0;
    }
    if (target.msaa_framebuffer) {
        glDeleteFramebuffers(1, &target.msaa_framebuffer);
        target.msaa_framebuffer = 0;
    }
    if (target.msaa_color_renderbuffer) {
        glDeleteRenderbuffers(1, &target.msaa_color_renderbuffer);
        target.msaa_color_renderbuffer = 0;
    }
    if (target.msaa_depth_renderbuffer) {
        glDeleteRenderbuffers(1, &target.msaa_depth_renderbuffer);
        target.msaa_depth_renderbuffer = 0;
    }

    target.samples = samples;

    // Create resolve texture (non-MSAA, for sampling)
    glGenTextures(1, &target.texture);
    glBindTexture(GL_TEXTURE_2D, target.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create depth renderbuffer for resolve framebuffer (needed for FBO completeness)
    glGenRenderbuffers(1, &target.depth_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, target.depth_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    // Create resolve framebuffer (non-MSAA)
    glGenFramebuffers(1, &target.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, target.depth_renderbuffer);

    if (samples > 1) {
        // Create MSAA color renderbuffer
        glGenRenderbuffers(1, &target.msaa_color_renderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, target.msaa_color_renderbuffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width, height);

        // Create MSAA depth renderbuffer
        glGenRenderbuffers(1, &target.msaa_depth_renderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, target.msaa_depth_renderbuffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);

        // Create MSAA framebuffer (for rendering)
        glGenFramebuffers(1, &target.msaa_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, target.msaa_framebuffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, target.msaa_color_renderbuffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, target.msaa_depth_renderbuffer);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            LogError("WebGL MSAA framebuffer incomplete: 0x%X", status);
        }
    }

    // Verify resolve framebuffer is complete
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LogError("WebGL framebuffer incomplete: 0x%X", status);
    }

    // Check for any GL errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        LogError("GL error after creating offscreen target: 0x%X", err);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
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
    attrs.antialias = traits->msaa ? EM_TRUE : EM_FALSE;
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
    } else {
        LogInfo("Using WebGL 2.0");
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

    // Create offscreen render targets
    CreateOffscreenTarget(g_gl.offscreen, g_gl.screen_size.x, g_gl.screen_size.y, samples);
    CreateOffscreenTarget(g_gl.ui_offscreen, g_gl.screen_size.x, g_gl.screen_size.y, samples);

    LogInfo("WebGL initialized: %dx%d, MSAA=%d", g_gl.screen_size.x, g_gl.screen_size.y, samples);
}

void ResizeWebGL(const Vec2Int& size) {
    if (size.x <= 0 || size.y <= 0)
        return;

    // Only update native size here - actual render target resize is handled by
    // PlatformSetRenderSize which knows about logical vs native size
    if (g_gl.native_screen_size != size) {
        LogInfo("ResizeWebGL native: %dx%d", size.x, size.y);
        g_gl.native_screen_size = size;
    }
}

void PlatformSetRenderSize(Vec2Int logical_size, Vec2Int native_size) {
    if (logical_size.x <= 0 || logical_size.y <= 0)
        return;

    // Skip if context is lost
    if (g_webgl.context_lost) {
        LogError("PlatformSetRenderSize: context lost, skipping");
        return;
    }

    bool native_changed = g_gl.native_screen_size != native_size;
    bool logical_changed = g_gl.screen_size != logical_size;

    g_gl.native_screen_size = native_size;

    // Recreate targets if logical size changed
    if (logical_changed) {
        LogInfo("Render targets: %dx%d -> %dx%d (native: %dx%d)",
                g_gl.screen_size.x, g_gl.screen_size.y,
                logical_size.x, logical_size.y,
                native_size.x, native_size.y);

        // Ensure context is current before modifying GL state
        emscripten_webgl_make_context_current(g_webgl.context);

        // Check actual drawing buffer size
        int draw_w, draw_h;
        emscripten_webgl_get_drawing_buffer_size(g_webgl.context, &draw_w, &draw_h);
        LogInfo("Drawing buffer size: %dx%d", draw_w, draw_h);

        // Finish any pending GL operations before destroying/recreating resources
        glFinish();

        g_gl.screen_size = logical_size;
        int samples = g_gl.offscreen.samples;
        CreateOffscreenTarget(g_gl.offscreen, logical_size.x, logical_size.y, samples);
        CreateOffscreenTarget(g_gl.ui_offscreen, logical_size.x, logical_size.y, samples);

        LogInfo("Render targets created successfully");
    } else if (native_changed) {
        LogInfo("Native size changed: %dx%d (logical: %dx%d unchanged)",
                native_size.x, native_size.y,
                logical_size.x, logical_size.y);
    }
}

static void DestroyOffscreenTarget(OffscreenTarget& target) {
    if (target.framebuffer) {
        glDeleteFramebuffers(1, &target.framebuffer);
        target.framebuffer = 0;
    }
    if (target.texture) {
        glDeleteTextures(1, &target.texture);
        target.texture = 0;
    }
    if (target.depth_renderbuffer) {
        glDeleteRenderbuffers(1, &target.depth_renderbuffer);
        target.depth_renderbuffer = 0;
    }
    if (target.msaa_framebuffer) {
        glDeleteFramebuffers(1, &target.msaa_framebuffer);
        target.msaa_framebuffer = 0;
    }
    if (target.msaa_color_renderbuffer) {
        glDeleteRenderbuffers(1, &target.msaa_color_renderbuffer);
        target.msaa_color_renderbuffer = 0;
    }
    if (target.msaa_depth_renderbuffer) {
        glDeleteRenderbuffers(1, &target.msaa_depth_renderbuffer);
        target.msaa_depth_renderbuffer = 0;
    }
}

void ShutdownWebGL() {
    // Delete offscreen targets
    DestroyOffscreenTarget(g_gl.offscreen);
    DestroyOffscreenTarget(g_gl.ui_offscreen);

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

// These functions use the Vulkan naming convention for compatibility
// with the existing platform abstraction
void InitRenderDriver(const RendererTraits* traits, const char* canvas_id) {
    InitWebGL(traits, canvas_id);
}

void ResizeRenderDriver(const Vec2Int& size) {
    ResizeWebGL(size);
}

void ShutdownRenderDriver() {
    ShutdownWebGL();
}

