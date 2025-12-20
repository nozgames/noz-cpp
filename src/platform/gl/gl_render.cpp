//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
//  Platform-independent OpenGL renderer implementation
//

#include "gl_internal.h"
#include <cassert>

#ifndef NOZ_PLATFORM_WEB
// Global function pointers - definitions (not needed on web, Emscripten provides them)
PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBINDBUFFERBASEPROC glBindBufferBase = nullptr;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = nullptr;
PFNGLBINDTEXTUREPROC glBindTexture = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLBLENDEQUATIONPROC glBlendEquation = nullptr;
PFNGLBLENDFUNCPROC glBlendFunc = nullptr;
PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLBUFFERSUBDATAPROC glBufferSubData = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;
PFNGLCLEARPROC glClear = nullptr;
PFNGLCLEARCOLORPROC glClearColor = nullptr;
PFNGLCLEARDEPTHFPROC glClearDepthf = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLCULLFACEPROC glCullFace = nullptr;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLDELETETEXTURESPROC glDeleteTextures = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
PFNGLDEPTHFUNCPROC glDepthFunc = nullptr;
PFNGLDEPTHMASKPROC glDepthMask = nullptr;
PFNGLDISABLEPROC glDisable = nullptr;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = nullptr;
PFNGLDRAWARRAYSPROC glDrawArrays = nullptr;
PFNGLDRAWELEMENTSPROC glDrawElements = nullptr;
PFNGLENABLEPROC glEnable = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
PFNGLFRONTFACEPROC glFrontFace = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = nullptr;
PFNGLGENTEXTURESPROC glGenTextures = nullptr;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap = nullptr;
PFNGLGETERRORPROC glGetError = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSTRINGPROC glGetString = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLPIXELSTOREIPROC glPixelStorei = nullptr;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = nullptr;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample = nullptr;
PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer = nullptr;
PFNGLSCISSORPROC glScissor = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLTEXIMAGE2DPROC glTexImage2D = nullptr;
PFNGLTEXPARAMETERIPROC glTexParameteri = nullptr;
PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;
PFNGLUNIFORM1IPROC glUniform1i = nullptr;
PFNGLUNIFORM2FPROC glUniform2f = nullptr;
PFNGLUNIFORM3FPROC glUniform3f = nullptr;
PFNGLUNIFORM4FPROC glUniform4f = nullptr;
PFNGLUNIFORMBLOCKBINDINGPROC glUniformBlockBinding = nullptr;
PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv = nullptr;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLVERTEXATTRIBIPOINTERPROC glVertexAttribIPointer = nullptr;
PFNGLVIEWPORTPROC glViewport = nullptr;
PFNGLCLIPCONTROLPROC glClipControl = nullptr;
PFNGLGETINTEGERVPROC glGetIntegerv = nullptr;
#endif // NOZ_PLATFORM_WEB

GLState g_gl = {};

void DrawTestQuad();

// Shared offscreen target management
void CreateOffscreenTarget(OffscreenTarget& target, int width, int height, int samples) {
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

    // Create depth renderbuffer for resolve framebuffer
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
            LogError("MSAA framebuffer incomplete: 0x%X", status);
        }
    }

    // Verify resolve framebuffer is complete
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LogError("Framebuffer incomplete: 0x%X", status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void DestroyOffscreenTarget(OffscreenTarget& target) {
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

void PlatformBeginRender() {
}

void PlatformBindSkeleton(const Mat3* bone_transforms, u8 bone_count) {
    float* dst = reinterpret_cast<float *>(g_gl.uniform_data[UNIFORM_BUFFER_SKELETON]);
    for (u8 i = 0; i < bone_count && i < MAX_BONES; i++) {
        const Mat3& m = bone_transforms[i];
        *dst++ = m.m[0]; *dst++ = m.m[3]; *dst++ = m.m[6]; *dst++ = 0.0f;
        *dst++ = m.m[1]; *dst++ = m.m[4]; *dst++ = m.m[7]; *dst++ = 0.0f;
        *dst++ = m.m[2]; *dst++ = m.m[5]; *dst++ = m.m[8]; *dst++ = 0.0f;
    }
    g_gl.ubo_dirty_flags |= (1 << UNIFORM_BUFFER_SKELETON);
}

void PlatformBindTransform(const Mat3& transform, float depth, float depth_scale) {
    ObjectBuffer* obj = reinterpret_cast<ObjectBuffer *>(g_gl.uniform_data[UNIFORM_BUFFER_OBJECT]);
    float* dst = obj->transform;
    *dst++ = transform.m[0]; *dst++ = transform.m[3]; *dst++ = transform.m[6]; *dst++ = 0.0f;
    *dst++ = transform.m[1]; *dst++ = transform.m[4]; *dst++ = transform.m[7]; *dst++ = 0.0f;
    *dst++ = transform.m[2]; *dst++ = transform.m[5]; *dst++ = transform.m[8]; *dst++ = 0.0f;
    obj->depth = depth;
    obj->depth_scale = depth_scale;
    obj->depth_min = g_gl.traits.min_depth;
    obj->depth_max = g_gl.traits.max_depth;
    g_gl.ubo_dirty_flags |= (1 << UNIFORM_BUFFER_OBJECT);
}

void PlatformBindVertexUserData(const u8* data, u32 size) {
    memcpy(g_gl.uniform_data[UNIFORM_BUFFER_VERTEX_USER], data, size);
    g_gl.ubo_dirty_flags |= (1 << UNIFORM_BUFFER_VERTEX_USER);
}

void PlatformBindFragmentUserData(const u8* data, u32 size) {
    memcpy(g_gl.uniform_data[UNIFORM_BUFFER_FRAGMENT_USER], data, size);
    g_gl.ubo_dirty_flags |= (1 << UNIFORM_BUFFER_FRAGMENT_USER);
}

void PlatformBindCamera(const Mat3& view_matrix) {
    float* dst = reinterpret_cast<float*>(g_gl.uniform_data[UNIFORM_BUFFER_CAMERA]);
    *dst++ = view_matrix.m[0]; *dst++ = view_matrix.m[3]; *dst++ = view_matrix.m[6]; *dst++ = 0.0f;
    *dst++ = view_matrix.m[1]; *dst++ = view_matrix.m[4]; *dst++ = view_matrix.m[7]; *dst++ = 0.0f;
    *dst++ = view_matrix.m[2]; *dst++ = view_matrix.m[5]; *dst++ = view_matrix.m[8]; *dst++ = 0.0f;
    g_gl.ubo_dirty_flags |= (1 << UNIFORM_BUFFER_CAMERA);
}

void PlatformBindColor(const Color& color, const Vec2& color_uv_offset, const Color& emission) {
    ColorBuffer* cb = reinterpret_cast<ColorBuffer*>(g_gl.uniform_data[UNIFORM_BUFFER_COLOR]);
    cb->color = color;
    cb->emission = emission;
    cb->uv_offset = color_uv_offset;
    g_gl.ubo_dirty_flags |= (1 << UNIFORM_BUFFER_COLOR);
}

void PlatformFree(PlatformBuffer* buffer) {
    if (!buffer) return;
    GLuint buf = static_cast<GLuint>(reinterpret_cast<uintptr_t>(buffer));
    glDeleteBuffers(1, &buf);
}

PlatformBuffer* PlatformCreateVertexBuffer(const MeshVertex* vertices, u16 vertex_count, const char* name, BufferFlags flags) {
    (void)name;
    assert(vertices);
    assert(vertex_count > 0);

    GLenum usage = (flags & BUFFER_FLAG_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(MeshVertex), vertices, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return (PlatformBuffer*)(uintptr_t)vbo;
}

PlatformBuffer* PlatformCreateIndexBuffer(const u16* indices, u16 index_count, const char* name, BufferFlags flags) {
    (void)name;
    assert(indices);
    assert(index_count > 0);

    GLenum usage = (flags & BUFFER_FLAG_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    GLuint ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(u16), indices, usage);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return (PlatformBuffer*)(uintptr_t)ibo;
}

void PlatformUpdateVertexBuffer(PlatformBuffer* buffer, const MeshVertex* vertices, u16 vertex_count) {
    assert(buffer);
    assert(vertices);
    GLuint vbo = (GLuint)(uintptr_t)buffer;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count * sizeof(MeshVertex), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void PlatformUpdateIndexBuffer(PlatformBuffer* buffer, const u16* indices, u16 index_count) {
    assert(buffer);
    assert(indices);
    GLuint ibo = (GLuint)(uintptr_t)buffer;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, index_count * sizeof(u16), indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void PlatformBindVertexBuffer(PlatformBuffer* buffer) {
    assert(buffer);
    GLuint vbo = (GLuint)(uintptr_t)buffer;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    g_gl.bound_vertex_buffer = vbo;

    // Set up vertex attributes for MeshVertex
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, depth));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, uv));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, normal));

    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 4, GL_INT, sizeof(MeshVertex), (void*)offsetof(MeshVertex, bone_indices));

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, bone_weights));
}

void PlatformBindIndexBuffer(PlatformBuffer* buffer) {
    assert(buffer);
    GLuint ibo = (GLuint)(uintptr_t)buffer;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    g_gl.bound_index_buffer = ibo;
}

void PlatformDrawIndexed(u16 index_count) {
    assert(index_count > 0);

    // Upload only uniform buffers that changed since last draw
    for (int i = 0; i < UNIFORM_BUFFER_COUNT; i++) {
        if (!(g_gl.ubo_dirty_flags & (1 << i)))
            continue;
        glBindBuffer(GL_UNIFORM_BUFFER, g_gl.ubos[i]);
        glBufferData(GL_UNIFORM_BUFFER, MAX_UNIFORM_BUFFER_SIZE, g_gl.uniform_data[i], GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, i, g_gl.ubos[i]);
    }
    g_gl.ubo_dirty_flags = 0;

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, nullptr);
}

PlatformTexture* PlatformCreateTexture(
    void* data,
    size_t width,
    size_t height,
    int channels,
    const SamplerOptions& sampler_options,
    const char* name) {
    (void)name;
    PlatformTexture* texture = new PlatformTexture();
    texture->size = {static_cast<i32>(width), static_cast<i32>(height)};
    texture->channels = channels;
    texture->sampler_options = sampler_options;

    glGenTextures(1, &texture->gl_texture);
    glBindTexture(GL_TEXTURE_2D, texture->gl_texture);

    GLenum format, internal_format;
    switch (channels) {
        case 1:
            format = GL_RED;
            internal_format = GL_R8;
            break;
        case 3:
            format = GL_RGB;
            internal_format = GL_RGB8;
            break;
        case 4:
        default:
            format = GL_RGBA;
            internal_format = GL_RGBA8;
            break;
    }

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internal_format,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        0,
        format,
        GL_UNSIGNED_BYTE,
        data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ToGL(sampler_options.filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ToGL(sampler_options.filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ToGL(sampler_options.clamp));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ToGL(sampler_options.clamp));

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

void PlatformFree(PlatformTexture* texture) {
    if (!texture) return;
    if (texture->gl_texture)
        glDeleteTextures(1, &texture->gl_texture);
    delete texture;
}

void PlatformBindTexture(PlatformTexture* texture, int slot) {
    GLuint tex_id = texture ? texture->gl_texture : 0;

    // Skip if texture already bound to this slot
    if (g_gl.bound_textures[slot] == tex_id)
        return;

    // Only switch texture unit if needed
    if (g_gl.current_texture_unit != slot) {
        glActiveTexture(GL_TEXTURE0 + slot);
        g_gl.current_texture_unit = slot;
    }

    glBindTexture(GL_TEXTURE_2D, tex_id);
    g_gl.bound_textures[slot] = tex_id;
}

void UpdateTexture(PlatformTexture* texture, void* data, const noz::Rect& rect) {
    if (!texture || !data) return;

    glBindTexture(GL_TEXTURE_2D, texture->gl_texture);

    GLenum format;
    switch (texture->channels) {
        case 1: format = GL_RED; break;
        case 3: format = GL_RGB; break;
        case 4:
        default: format = GL_RGBA; break;
    }
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        static_cast<GLint>(rect.x),
        static_cast<GLint>(rect.y),
        static_cast<GLsizei>(rect.width),
        static_cast<GLsizei>(rect.height),
        format,
        GL_UNSIGNED_BYTE,
        data);

    glBindTexture(GL_TEXTURE_2D, 0);
}

Vec2Int GetTextureSize(PlatformTexture* texture) {
    return texture ? texture->size : VEC2INT_ZERO;
}

void PlatformBeginScenePass(Color clear_color) {
    GLuint fb = g_gl.offscreen.msaa_framebuffer
        ? g_gl.offscreen.msaa_framebuffer
        : g_gl.offscreen.framebuffer;

    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glViewport(0, 0, g_gl.screen_size.x, g_gl.screen_size.y);
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    glClearDepthf(1.0f);
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PlatformEndScenePass() {
    if (!g_gl.offscreen.msaa_framebuffer)
        return;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, g_gl.offscreen.msaa_framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_gl.offscreen.framebuffer);
    glBlitFramebuffer(
        0, 0, g_gl.screen_size.x, g_gl.screen_size.y,
        0, 0, g_gl.screen_size.x, g_gl.screen_size.y,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PlatformBeginPostProcPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_gl.screen_size.x, g_gl.screen_size.y);
    glDisable(GL_DEPTH_TEST);
}

void PlatformEndPostProcPass() {
}

void PlatformEnablePostProcess(bool enabled) {
    g_gl.postprocess_enabled = enabled;
}

void PlatformBeginUIPass() {
    // No longer used - UI renders to same target as scene
}

void PlatformEndUIPass() {
    // No longer used - UI renders to same target as scene
}

void PlatformBeginCompositePass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // Use native screen size for composite output (may differ from logical size when rotated)
    Vec2Int native_size = g_gl.native_screen_size.x > 0 ? g_gl.native_screen_size : g_gl.screen_size;
    glViewport(0, 0, native_size.x, native_size.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
}

void PlatformEndCompositePass() {
}

void PlatformBindSceneTexture() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_gl.offscreen.texture);
    PlatformBindCamera(Translate(Vec2{0, 1}) * Scale(Vec2{1, -1}));

    // Apply 90° CW rotation to object transform if screen is rotated
    if (IsScreenRotated()) {
        Mat3 rotation = Mat3{.m = {
             0, 1, 0,
            -1, 0, 0,
             0, 0, 1
        }};
        PlatformBindTransform(rotation, 0.0f, 1.0f);
    }
}

void PlatformBindUITexture() {
    // No longer used - UI renders to same target as scene
}

void PlatformBindSceneTextureOnly() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_gl.offscreen.texture);
}

void PlatformBindUITextureOnly() {
    // No longer used - UI renders to same target as scene
}

void PlatformSetViewport(const noz::Rect& viewport) {
    if (viewport.width <= 0 || viewport.height <= 0) {
        glViewport(0, 0, g_gl.screen_size.x, g_gl.screen_size.y);
    } else {
        glViewport((GLint)viewport.x, (GLint)viewport.y,
                   (GLsizei)viewport.width, (GLsizei)viewport.height);
    }
}

static GLuint CompileGLShader(GLenum type, const char* source, u32 source_size, const char* name) {
    if (!source || source_size == 0)
        return 0;

    GLuint shader = glCreateShader(type);
    if (!shader)
        return 0;

    GLint length = (GLint)source_size;
    glShaderSource(shader, 1, &source, &length);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, info_log);
        const char* type_str = (type == GL_VERTEX_SHADER) ? "vertex" :
                               (type == GL_FRAGMENT_SHADER) ? "fragment" : "geometry";
        LogError("GLSL %s shader compile error (%s): %s", type_str, name ? name : "unknown", info_log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

PlatformShader* PlatformCreateShader(
    const void* vertex,
    u32 vertex_size,
    const void* fragment,
    u32 fragment_size,
    ShaderFlags flags,
    const char* name)
{
    PlatformShader* shader = new PlatformShader();
    shader->program = 0;
    shader->flags = flags;

    for (int i = 0; i < UNIFORM_BUFFER_COUNT; i++)
        shader->uniform_block_indices[i] = GL_INVALID_INDEX;

    if (!vertex || !fragment)
        return shader;

    GLuint vert_shader = CompileGLShader(GL_VERTEX_SHADER, (const char*)vertex, vertex_size, name);
    if (!vert_shader) {
        delete shader;
        return nullptr;
    }

    GLuint frag_shader = CompileGLShader(GL_FRAGMENT_SHADER, (const char*)fragment, fragment_size, name);
    if (!frag_shader) {
        glDeleteShader(vert_shader);
        delete shader;
        return nullptr;
    }

    shader->program = glCreateProgram();
    glAttachShader(shader->program, vert_shader);
    glAttachShader(shader->program, frag_shader);

    glLinkProgram(shader->program);

    GLint success;
    glGetProgramiv(shader->program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[1024];
        glGetProgramInfoLog(shader->program, 1024, nullptr, info_log);
        LogError("GLSL program link error (%s): %s", name ? name : "unknown", info_log);
        glDeleteProgram(shader->program);
        shader->program = 0;
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    if (shader->program) {
        const char* uniform_names[] = {
            "CameraBuffer",
            "ObjectBuffer",
            "SkeletonBuffer",
            "VertexUserBuffer",
            "ColorBuffer",
            "FragmentUserBuffer"
        };

        for (int i = 0; i < UNIFORM_BUFFER_COUNT; i++)
            shader->uniform_block_indices[i] = glGetUniformBlockIndex(shader->program, uniform_names[i]);
    }

    return shader;
}

void PlatformBindShader(PlatformShader* shader) {
    if (!shader || !shader->program) return;

    // Skip if shader already bound
    if (shader->program == g_gl.current_program)
        return;

    glUseProgram(shader->program);
    g_gl.current_program = shader->program;

    // Apply shader flags only if they changed
    ShaderFlags flags = shader->flags;
    ShaderFlags changed = flags ^ g_gl.current_shader_flags;

    // Depth testing - only update if depth-related flags changed
    if (changed & (SHADER_FLAGS_DEPTH | SHADER_FLAGS_DEPTH_LESS)) {
        bool depth_test = (flags & SHADER_FLAGS_DEPTH) != 0;
        if (depth_test) {
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            bool depth_less = (flags & SHADER_FLAGS_DEPTH_LESS) != 0;
            glDepthFunc(depth_less ? GL_LESS : GL_LEQUAL);
        } else {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
        }
    }

    // Blending - only update if blend-related flags changed
    if (changed & (SHADER_FLAGS_PREMULTIPLIED_ALPHA | SHADER_FLAGS_UI_COMPOSITE | SHADER_FLAGS_BLEND)) {
        bool is_premultiplied = (flags & SHADER_FLAGS_PREMULTIPLIED_ALPHA) != 0;
        bool is_ui_composite = (flags & SHADER_FLAGS_UI_COMPOSITE) != 0;
        bool is_blend = (flags & SHADER_FLAGS_BLEND) != 0;

        if (is_premultiplied || is_ui_composite) {
            // Premultiplied alpha: src * 1 + dst * (1 - src_alpha)
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        } else if (is_blend) {
            // Standard alpha blending
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glDisable(GL_BLEND);
        }
    }

    g_gl.current_shader_flags = flags;

    for (int i = 0; i < UNIFORM_BUFFER_COUNT; i++)
        if (shader->uniform_block_indices[i] != GL_INVALID_INDEX)
            glUniformBlockBinding(shader->program, shader->uniform_block_indices[i], i);
}

void PlatformFree(PlatformShader* shader) {
    if (!shader) return;

    if (shader->program)
        glDeleteProgram(shader->program);

    delete shader;
}

void PlatformEndSwapChain() {
    // Unbind framebuffer before binding its texture to avoid feedback loop
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
