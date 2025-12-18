//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
//  Internal shared definitions for GLES renderer
//

#pragma once

#include "gl_render.h"
#include "../../platform.h"
#include "../../internal.h"

#include <cstring>

struct Buffer {};

enum UniformBufferType {
    UNIFORM_BUFFER_CAMERA,
    UNIFORM_BUFFER_OBJECT,
    UNIFORM_BUFFER_SKELETON,
    UNIFORM_BUFFER_VERTEX_USER,
    UNIFORM_BUFFER_COLOR,
    UNIFORM_BUFFER_FRAGMENT_USER,
    UNIFORM_BUFFER_COUNT
};

constexpr int VK_MAX_TEXTURES = 128;
constexpr int VK_MAX_UNIFORM_BUFFERS = 8192;
constexpr u32 VK_DYNAMIC_UNIFORM_BUFFER_SIZE = MAX_UNIFORM_BUFFER_SIZE * VK_MAX_UNIFORM_BUFFERS;

inline GLint ToGL(TextureFilter filter) {
    return filter == TEXTURE_FILTER_LINEAR ? GL_LINEAR : GL_NEAREST;
}

inline GLint ToGL(TextureClamp clamp) {
    return clamp == TEXTURE_CLAMP_REPEAT ? GL_REPEAT : GL_CLAMP_TO_EDGE;
}

struct PlatformTexture {
    GLuint gl_texture;
    SamplerOptions sampler_options;
    Vec2Int size;
    i32 channels;
};

struct PlatformShader {
    GLuint program;
    GLuint uniform_block_indices[UNIFORM_BUFFER_COUNT];
    ShaderFlags flags;
};

struct ObjectBuffer {
    float transform[12];
    float depth;
    float depth_scale;
    float depth_min;
    float depth_max;
};

struct ColorBuffer {
    Color color;
    Color emission;
    Vec2 uv_offset;
    Vec2 padding;
};

struct OffscreenTarget {
    GLuint texture;              // Resolved (non-MSAA) texture for sampling
    GLuint framebuffer;          // Resolve framebuffer (non-MSAA)
    GLuint depth_renderbuffer;   // Non-MSAA depth (for resolve FB)

    // MSAA resources
    GLuint msaa_framebuffer;     // MSAA framebuffer for rendering
    GLuint msaa_color_renderbuffer;
    GLuint msaa_depth_renderbuffer;
    int samples;
};

// Platform-independent GL state
struct GLState {
    GLuint current_vao;
    GLuint current_program;
    GLuint bound_vertex_buffer;
    GLuint bound_index_buffer;

    Vec2Int screen_size;

    // Uniform buffer data (CPU-side, uploaded per-draw)
    u8 uniform_data[UNIFORM_BUFFER_COUNT][MAX_UNIFORM_BUFFER_SIZE];

    // GPU-side Uniform Buffer Objects
    GLuint ubos[UNIFORM_BUFFER_COUNT];

    // Dirty flags for uniform buffers (1 bit per buffer)
    u32 ubo_dirty_flags;

    // State caching for textures
    GLuint bound_textures[8];
    int current_texture_unit;

    // State caching for shader flags (blend/depth state)
    ShaderFlags current_shader_flags;

    // Offscreen rendering
    OffscreenTarget offscreen;
    OffscreenTarget ui_offscreen;
    bool postprocess_enabled;

    float depth_conversion_factor;
    RendererTraits traits;
};

// Global GL state - defined in gles_render.cpp
extern GLState g_gl;

// Debug test quad
void DrawTestQuad();
