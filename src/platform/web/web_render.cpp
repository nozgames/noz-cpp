//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
//  Web/Emscripten WebGL renderer implementation
//

#include "../gles/gles_internal.h"

#include <emscripten.h>
#include <emscripten/html5.h>

// Web-specific state
static struct {
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
    const char* canvas_id;
} g_webgl = {};

// Note: The gles_render.cpp provides most of the rendering implementation.
// This file provides the WebGL-specific initialization and frame management.

static void CreateOffscreenTarget(OffscreenTarget& target, int width, int height) {
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

    // Create color texture
    glGenTextures(1, &target.texture);
    glBindTexture(GL_TEXTURE_2D, target.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create depth renderbuffer
    glGenRenderbuffers(1, &target.depth_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, target.depth_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    // Create framebuffer
    glGenFramebuffers(1, &target.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, target.depth_renderbuffer);

    // Verify framebuffer is complete
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LogError("WebGL framebuffer incomplete: 0x%X", status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
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

    // Get initial canvas size
    int width, height;
    emscripten_webgl_get_drawing_buffer_size(g_webgl.context, &width, &height);
    g_gl.screen_size = {width, height};

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

    // Create offscreen render targets
    CreateOffscreenTarget(g_gl.offscreen, g_gl.screen_size.x, g_gl.screen_size.y);
    CreateOffscreenTarget(g_gl.ui_offscreen, g_gl.screen_size.x, g_gl.screen_size.y);

    LogInfo("WebGL initialized: %dx%d", g_gl.screen_size.x, g_gl.screen_size.y);
}

void ResizeWebGL(const Vec2Int& size) {
    if (size.x <= 0 || size.y <= 0)
        return;

    g_gl.screen_size = size;
    glViewport(0, 0, size.x, size.y);

    // Recreate offscreen targets at new size
    CreateOffscreenTarget(g_gl.offscreen, size.x, size.y);
    CreateOffscreenTarget(g_gl.ui_offscreen, size.x, size.y);

    LogInfo("WebGL resized: %dx%d", size.x, size.y);
}

void ShutdownWebGL() {
    // Delete offscreen targets
    if (g_gl.offscreen.framebuffer) {
        glDeleteFramebuffers(1, &g_gl.offscreen.framebuffer);
        glDeleteTextures(1, &g_gl.offscreen.texture);
        glDeleteRenderbuffers(1, &g_gl.offscreen.depth_renderbuffer);
    }
    if (g_gl.ui_offscreen.framebuffer) {
        glDeleteFramebuffers(1, &g_gl.ui_offscreen.framebuffer);
        glDeleteTextures(1, &g_gl.ui_offscreen.texture);
        glDeleteRenderbuffers(1, &g_gl.ui_offscreen.depth_renderbuffer);
    }

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

void PlatformEndRenderFrame() {
    // WebGL handles buffer swapping automatically
    // Nothing to do here - the browser composites at the end of the frame
}

// These functions use the Vulkan naming convention for compatibility
// with the existing platform abstraction
void InitVulkan(const RendererTraits* traits, const char* canvas_id) {
    InitWebGL(traits, canvas_id);
}

void ResizeVulkan(const Vec2Int& size) {
    ResizeWebGL(size);
}

void ShutdownVulkan() {
    ShutdownWebGL();
}

void WaitVulkan() {
    // No explicit sync needed in WebGL
}
