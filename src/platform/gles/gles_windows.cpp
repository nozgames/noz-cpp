//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
//  Windows OpenGL renderer implementation
//

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "gles_render.h"
#include "../../platform.h"

#include <vector>
#include <cassert>
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

constexpr int MAX_TEXTURES = 128;
constexpr int MAX_UNIFORM_BUFFERS = 8192;
constexpr u32 DYNAMIC_UNIFORM_BUFFER_SIZE = MAX_UNIFORM_BUFFER_SIZE * MAX_UNIFORM_BUFFERS;

static GLint ToGL(TextureFilter filter) {
    return filter == TEXTURE_FILTER_LINEAR ? GL_LINEAR : GL_NEAREST;
}

static GLint ToGL(TextureClamp clamp) {
    return clamp == TEXTURE_CLAMP_REPEAT ? GL_REPEAT : GL_CLAMP_TO_EDGE;
}

struct platform::Texture {
    GLuint gl_texture;
    SamplerOptions sampler_options;
    Vec2Int size;
    i32 channels;
};

struct platform::Shader {
    GLuint program;
    GLint uniform_locations[UNIFORM_BUFFER_COUNT];
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

struct GLDynamicBuffer {
    GLuint buffer;
    void* mapped_ptr;
    u32 offset;
    u32 size;
};

struct OffscreenTarget {
    GLuint texture;
    GLuint framebuffer;
    GLuint depth_renderbuffer;
};

struct GLRenderer {
    RendererTraits traits;
    HWND hwnd;
    HDC hdc;
    HGLRC hglrc;
    HMODULE library;

    GLuint current_vao;
    GLuint current_program;
    GLuint bound_vertex_buffer;
    GLuint bound_index_buffer;

    Vec2Int screen_size;

    // Uniform buffer data (CPU-side, uploaded per-draw)
    u8 uniform_data[UNIFORM_BUFFER_COUNT][MAX_UNIFORM_BUFFER_SIZE];

    // Offscreen rendering
    OffscreenTarget offscreen;
    OffscreenTarget ui_offscreen;
    bool postprocess_enabled;

    float depth_conversion_factor;
};

static GLRenderer g_gl = {};

// WGL function pointers
typedef HGLRC WINAPI wglCreateContext_t(HDC hdc);
typedef BOOL WINAPI wglDeleteContext_t(HGLRC hglrc);
typedef BOOL WINAPI wglMakeCurrent_t(HDC hdc, HGLRC hglrc);
typedef PROC WINAPI wglGetProcAddress_t(LPCSTR lpszProc);
typedef HGLRC WINAPI wglCreateContextAttribsARB_t(HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL WINAPI wglChoosePixelFormatARB_t(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef BOOL WINAPI wglSwapIntervalEXT_t(int interval);

static wglCreateContext_t* wglCreateContext_ptr = nullptr;
static wglDeleteContext_t* wglDeleteContext_ptr = nullptr;
static wglMakeCurrent_t* wglMakeCurrent_ptr = nullptr;
static wglGetProcAddress_t* wglGetProcAddress_ptr = nullptr;
static wglCreateContextAttribsARB_t* wglCreateContextAttribsARB_ptr = nullptr;
static wglChoosePixelFormatARB_t* wglChoosePixelFormatARB_ptr = nullptr;
static wglSwapIntervalEXT_t* wglSwapIntervalEXT_ptr = nullptr;

// WGL constants
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
#define WGL_SAMPLE_BUFFERS_ARB            0x2041
#define WGL_SAMPLES_ARB                   0x2042

static void* GetGLProcAddress(const char* name)
{
    void* proc = (void*)wglGetProcAddress_ptr(name);
    if (proc == nullptr || proc == (void*)0x1 || proc == (void*)0x2 || proc == (void*)0x3 || proc == (void*)-1)
    {
        proc = (void*)GetProcAddress(g_gl.library, name);
    }
    return proc;
}

bool LoadGLESLibrary()
{
    g_gl.library = LoadLibraryA("opengl32.dll");
    if (!g_gl.library)
        return false;

    wglCreateContext_ptr = (wglCreateContext_t*)GetProcAddress(g_gl.library, "wglCreateContext");
    wglDeleteContext_ptr = (wglDeleteContext_t*)GetProcAddress(g_gl.library, "wglDeleteContext");
    wglMakeCurrent_ptr = (wglMakeCurrent_t*)GetProcAddress(g_gl.library, "wglMakeCurrent");
    wglGetProcAddress_ptr = (wglGetProcAddress_t*)GetProcAddress(g_gl.library, "wglGetProcAddress");

    if (!wglCreateContext_ptr || !wglDeleteContext_ptr || !wglMakeCurrent_ptr || !wglGetProcAddress_ptr)
        return false;

    return true;
}

void LoadGLESFunctions()
{
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)GetGLProcAddress("glActiveTexture");
    glAttachShader = (PFNGLATTACHSHADERPROC)GetGLProcAddress("glAttachShader");
    glBindBuffer = (PFNGLBINDBUFFERPROC)GetGLProcAddress("glBindBuffer");
    glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)GetGLProcAddress("glBindBufferBase");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)GetGLProcAddress("glBindFramebuffer");
    glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)GetGLProcAddress("glBindRenderbuffer");
    glBindTexture = (PFNGLBINDTEXTUREPROC)GetGLProcAddress("glBindTexture");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)GetGLProcAddress("glBindVertexArray");
    glBlendEquation = (PFNGLBLENDEQUATIONPROC)GetGLProcAddress("glBlendEquation");
    glBlendFunc = (PFNGLBLENDFUNCPROC)GetGLProcAddress("glBlendFunc");
    glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)GetGLProcAddress("glBlendFuncSeparate");
    glBufferData = (PFNGLBUFFERDATAPROC)GetGLProcAddress("glBufferData");
    glBufferSubData = (PFNGLBUFFERSUBDATAPROC)GetGLProcAddress("glBufferSubData");
    glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)GetGLProcAddress("glCheckFramebufferStatus");
    glClear = (PFNGLCLEARPROC)GetGLProcAddress("glClear");
    glClearColor = (PFNGLCLEARCOLORPROC)GetGLProcAddress("glClearColor");
    glClearDepthf = (PFNGLCLEARDEPTHFPROC)GetGLProcAddress("glClearDepthf");
    glCompileShader = (PFNGLCOMPILESHADERPROC)GetGLProcAddress("glCompileShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)GetGLProcAddress("glCreateProgram");
    glCreateShader = (PFNGLCREATESHADERPROC)GetGLProcAddress("glCreateShader");
    glCullFace = (PFNGLCULLFACEPROC)GetGLProcAddress("glCullFace");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)GetGLProcAddress("glDeleteBuffers");
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)GetGLProcAddress("glDeleteFramebuffers");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)GetGLProcAddress("glDeleteProgram");
    glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)GetGLProcAddress("glDeleteRenderbuffers");
    glDeleteShader = (PFNGLDELETESHADERPROC)GetGLProcAddress("glDeleteShader");
    glDeleteTextures = (PFNGLDELETETEXTURESPROC)GetGLProcAddress("glDeleteTextures");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)GetGLProcAddress("glDeleteVertexArrays");
    glDepthFunc = (PFNGLDEPTHFUNCPROC)GetGLProcAddress("glDepthFunc");
    glDepthMask = (PFNGLDEPTHMASKPROC)GetGLProcAddress("glDepthMask");
    glDisable = (PFNGLDISABLEPROC)GetGLProcAddress("glDisable");
    glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)GetGLProcAddress("glDisableVertexAttribArray");
    glDrawArrays = (PFNGLDRAWARRAYSPROC)GetGLProcAddress("glDrawArrays");
    glDrawElements = (PFNGLDRAWELEMENTSPROC)GetGLProcAddress("glDrawElements");
    glEnable = (PFNGLENABLEPROC)GetGLProcAddress("glEnable");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)GetGLProcAddress("glEnableVertexAttribArray");
    glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)GetGLProcAddress("glFramebufferRenderbuffer");
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)GetGLProcAddress("glFramebufferTexture2D");
    glFrontFace = (PFNGLFRONTFACEPROC)GetGLProcAddress("glFrontFace");
    glGenBuffers = (PFNGLGENBUFFERSPROC)GetGLProcAddress("glGenBuffers");
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)GetGLProcAddress("glGenFramebuffers");
    glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)GetGLProcAddress("glGenRenderbuffers");
    glGenTextures = (PFNGLGENTEXTURESPROC)GetGLProcAddress("glGenTextures");
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)GetGLProcAddress("glGenVertexArrays");
    glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)GetGLProcAddress("glGenerateMipmap");
    glGetError = (PFNGLGETERRORPROC)GetGLProcAddress("glGetError");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)GetGLProcAddress("glGetProgramInfoLog");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)GetGLProcAddress("glGetProgramiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)GetGLProcAddress("glGetShaderInfoLog");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)GetGLProcAddress("glGetShaderiv");
    glGetString = (PFNGLGETSTRINGPROC)GetGLProcAddress("glGetString");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)GetGLProcAddress("glGetUniformLocation");
    glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)GetGLProcAddress("glGetUniformBlockIndex");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)GetGLProcAddress("glLinkProgram");
    glPixelStorei = (PFNGLPIXELSTOREIPROC)GetGLProcAddress("glPixelStorei");
    glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)GetGLProcAddress("glRenderbufferStorage");
    glScissor = (PFNGLSCISSORPROC)GetGLProcAddress("glScissor");
    glShaderSource = (PFNGLSHADERSOURCEPROC)GetGLProcAddress("glShaderSource");
    glTexImage2D = (PFNGLTEXIMAGE2DPROC)GetGLProcAddress("glTexImage2D");
    glTexParameteri = (PFNGLTEXPARAMETERIPROC)GetGLProcAddress("glTexParameteri");
    glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)GetGLProcAddress("glTexSubImage2D");
    glUniform1f = (PFNGLUNIFORM1FPROC)GetGLProcAddress("glUniform1f");
    glUniform1i = (PFNGLUNIFORM1IPROC)GetGLProcAddress("glUniform1i");
    glUniform2f = (PFNGLUNIFORM2FPROC)GetGLProcAddress("glUniform2f");
    glUniform3f = (PFNGLUNIFORM3FPROC)GetGLProcAddress("glUniform3f");
    glUniform4f = (PFNGLUNIFORM4FPROC)GetGLProcAddress("glUniform4f");
    glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)GetGLProcAddress("glUniformBlockBinding");
    glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)GetGLProcAddress("glUniformMatrix3fv");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)GetGLProcAddress("glUniformMatrix4fv");
    glUseProgram = (PFNGLUSEPROGRAMPROC)GetGLProcAddress("glUseProgram");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)GetGLProcAddress("glVertexAttribPointer");
    glViewport = (PFNGLVIEWPORTPROC)GetGLProcAddress("glViewport");

    // WGL extensions
    wglCreateContextAttribsARB_ptr = (wglCreateContextAttribsARB_t*)GetGLProcAddress("wglCreateContextAttribsARB");
    wglChoosePixelFormatARB_ptr = (wglChoosePixelFormatARB_t*)GetGLProcAddress("wglChoosePixelFormatARB");
    wglSwapIntervalEXT_ptr = (wglSwapIntervalEXT_t*)GetGLProcAddress("wglSwapIntervalEXT");
}

void UnloadGLESLibrary()
{
    if (g_gl.library)
    {
        FreeLibrary(g_gl.library);
        g_gl.library = nullptr;
    }
}

static bool CreateOffscreenTarget(OffscreenTarget* target, int width, int height) {
    glGenFramebuffers(1, &target->framebuffer);
    glGenTextures(1, &target->texture);
    glGenRenderbuffers(1, &target->depth_renderbuffer);

    glBindTexture(GL_TEXTURE_2D, target->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindRenderbuffer(GL_RENDERBUFFER, target->depth_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, target->framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target->texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, target->depth_renderbuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

static void DestroyOffscreenTarget(OffscreenTarget* target) {
    if (target->framebuffer) {
        glDeleteFramebuffers(1, &target->framebuffer);
        target->framebuffer = 0;
    }
    if (target->texture) {
        glDeleteTextures(1, &target->texture);
        target->texture = 0;
    }
    if (target->depth_renderbuffer) {
        glDeleteRenderbuffers(1, &target->depth_renderbuffer);
        target->depth_renderbuffer = 0;
    }
}

// Platform render functions implementation
void platform::BeginRenderFrame() {
    // Reset state for new frame
}

void platform::EndRenderFrame() {
    SwapBuffers(g_gl.hdc);
}

void platform::BindSkeleton(const Mat3* bone_transforms, u8 bone_count) {
    // Copy bone transform data to uniform buffer
    float* dst = (float*)g_gl.uniform_data[UNIFORM_BUFFER_SKELETON];
    for (u8 i = 0; i < bone_count && i < MAX_BONES; i++) {
        const Mat3& m = bone_transforms[i];
        *dst++ = m.m[0]; *dst++ = m.m[1]; *dst++ = m.m[2]; *dst++ = 0.0f;
        *dst++ = m.m[3]; *dst++ = m.m[4]; *dst++ = m.m[5]; *dst++ = 0.0f;
        *dst++ = m.m[6]; *dst++ = m.m[7]; *dst++ = m.m[8]; *dst++ = 0.0f;
    }
}

void platform::BindTransform(const Mat3& transform, float depth, float depth_scale) {
    ObjectBuffer* obj = (ObjectBuffer*)g_gl.uniform_data[UNIFORM_BUFFER_OBJECT];
    float* dst = obj->transform;
    *dst++ = transform.m[0]; *dst++ = transform.m[1]; *dst++ = transform.m[2]; *dst++ = 0.0f;
    *dst++ = transform.m[3]; *dst++ = transform.m[4]; *dst++ = transform.m[5]; *dst++ = 0.0f;
    *dst++ = transform.m[6]; *dst++ = transform.m[7]; *dst++ = transform.m[8]; *dst++ = 0.0f;
    obj->depth = depth;
    obj->depth_scale = depth_scale;
    obj->depth_min = 0.0f;
    obj->depth_max = 1.0f;
}

void platform::BindVertexUserData(const u8* data, u32 size) {
    memcpy(g_gl.uniform_data[UNIFORM_BUFFER_VERTEX_USER], data, size);
}

void platform::BindFragmentUserData(const u8* data, u32 size) {
    memcpy(g_gl.uniform_data[UNIFORM_BUFFER_FRAGMENT_USER], data, size);
}

void platform::BindCamera(const Mat3& view_matrix) {
    float* dst = (float*)g_gl.uniform_data[UNIFORM_BUFFER_CAMERA];
    *dst++ = view_matrix.m[0]; *dst++ = view_matrix.m[1]; *dst++ = view_matrix.m[2]; *dst++ = 0.0f;
    *dst++ = view_matrix.m[3]; *dst++ = view_matrix.m[4]; *dst++ = view_matrix.m[5]; *dst++ = 0.0f;
    *dst++ = view_matrix.m[6]; *dst++ = view_matrix.m[7]; *dst++ = view_matrix.m[8]; *dst++ = 0.0f;
}

void platform::BindColor(const Color& color, const Vec2& color_uv_offset, const Color& emission) {
    ColorBuffer* cb = (ColorBuffer*)g_gl.uniform_data[UNIFORM_BUFFER_COLOR];
    cb->color = color;
    cb->emission = emission;
    cb->uv_offset = color_uv_offset;
}

void platform::DestroyBuffer(Buffer* buffer) {
    if (buffer) {
        GLuint buf = (GLuint)(uintptr_t)buffer;
        glDeleteBuffers(1, &buf);
    }
}

platform::Buffer* platform::CreateVertexBuffer(const MeshVertex* vertices, u16 vertex_count, const char* name) {
    assert(vertices);
    assert(vertex_count > 0);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(MeshVertex), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return (Buffer*)(uintptr_t)vbo;
}

platform::Buffer* platform::CreateIndexBuffer(const u16* indices, u16 index_count, const char* name) {
    assert(indices);
    assert(index_count > 0);

    GLuint ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(u16), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return (Buffer*)(uintptr_t)ibo;
}

void platform::BindVertexBuffer(Buffer* buffer) {
    assert(buffer);
    GLuint vbo = (GLuint)(uintptr_t)buffer;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    g_gl.bound_vertex_buffer = vbo;

    // Set up vertex attributes for MeshVertex
    // position (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, position));

    // depth (float)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, depth));

    // uv (vec2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, uv));

    // normal (vec2)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, normal));

    // bone_indices (ivec4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_INT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, bone_indices));

    // bone_weights (vec4)
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, bone_weights));
}

void platform::BindIndexBuffer(Buffer* buffer) {
    assert(buffer);
    GLuint ibo = (GLuint)(uintptr_t)buffer;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    g_gl.bound_index_buffer = ibo;
}

void platform::DrawIndexed(u16 index_count) {
    assert(index_count > 0);
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, nullptr);
}

platform::Texture* platform::CreateTexture(
    void* data,
    size_t width,
    size_t height,
    int channels,
    const SamplerOptions& sampler_options,
    const char* name)
{
    platform::Texture* texture = new platform::Texture();
    texture->size = {(i32)width, (i32)height};
    texture->channels = channels;
    texture->sampler_options = sampler_options;

    glGenTextures(1, &texture->gl_texture);
    glBindTexture(GL_TEXTURE_2D, texture->gl_texture);

    GLenum format = (channels == 1) ? GL_LUMINANCE : GL_RGBA;
    GLenum internal_format = (channels == 1) ? GL_LUMINANCE : GL_RGBA8;

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, (GLsizei)width, (GLsizei)height, 0, format, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ToGL(sampler_options.filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ToGL(sampler_options.filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ToGL(sampler_options.clamp));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ToGL(sampler_options.clamp));

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

void platform::DestroyTexture(Texture* texture) {
    if (texture) {
        if (texture->gl_texture) {
            glDeleteTextures(1, &texture->gl_texture);
        }
        delete texture;
    }
}

void platform::BindTexture(Texture* texture, int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    if (texture) {
        glBindTexture(GL_TEXTURE_2D, texture->gl_texture);
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void platform::BeginRenderPass(Color clear_color) {
    if (g_gl.postprocess_enabled) {
        glBindFramebuffer(GL_FRAMEBUFFER, g_gl.offscreen.framebuffer);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glViewport(0, 0, g_gl.screen_size.x, g_gl.screen_size.y);
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void platform::EndRenderPass() {
    // Render pass ended
}

void platform::BeginPostProcessPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_gl.screen_size.x, g_gl.screen_size.y);
    glDisable(GL_DEPTH_TEST);
}

void platform::EndPostProcessPass() {
    // Post process pass ended
}

void platform::SetPostProcessEnabled(bool enabled) {
    g_gl.postprocess_enabled = enabled;
}

void platform::BeginUIPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, g_gl.ui_offscreen.framebuffer);
    glViewport(0, 0, g_gl.screen_size.x, g_gl.screen_size.y);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void platform::BindOffscreenTexture() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_gl.offscreen.texture);
}

void platform::BindUIOffscreenTexture() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_gl.ui_offscreen.texture);
}

void platform::EndSwapchainPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void platform::SetViewport(const noz::Rect& viewport) {
    if (viewport.size.x <= 0 || viewport.size.y <= 0) {
        glViewport(0, 0, g_gl.screen_size.x, g_gl.screen_size.y);
    } else {
        glViewport((GLint)viewport.position.x, (GLint)viewport.position.y,
                   (GLsizei)viewport.size.x, (GLsizei)viewport.size.y);
    }
}

platform::Shader* platform::CreateShader(
    const void* vertex_code,
    u32 vertex_code_size,
    const void* geometry_code,
    u32 geometry_code_size,
    const void* fragment_code,
    u32 fragment_code_size,
    ShaderFlags flags,
    const char* name)
{
    // Note: OpenGL uses GLSL text, but this engine provides SPIR-V
    // For a full implementation, you'd need SPIRV-Cross to convert SPIR-V to GLSL
    // For now, we create a placeholder shader

    platform::Shader* shader = new platform::Shader();
    shader->program = 0;

    // Initialize uniform locations to -1 (not found)
    for (int i = 0; i < UNIFORM_BUFFER_COUNT; i++) {
        shader->uniform_locations[i] = -1;
    }

    return shader;
}

void platform::BindShader(Shader* shader) {
    if (shader && shader->program) {
        glUseProgram(shader->program);
        g_gl.current_program = shader->program;
    }
}

void platform::DestroyShader(Shader* shader) {
    if (shader) {
        if (shader->program) {
            glDeleteProgram(shader->program);
        }
        delete shader;
    }
}
