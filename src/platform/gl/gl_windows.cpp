//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
//  Windows-specific OpenGL renderer implementation
//

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "gl_internal.h"

// Windows-specific extensions to GLState
static struct {
    HWND hwnd;
    HDC hdc;
    HGLRC hglrc;
    HMODULE library;
} g_wgl = {};

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

static void* GetGLProcAddress(const char* name) {
    void* proc = (void*)wglGetProcAddress_ptr(name);
    if (proc == nullptr || proc == (void*)0x1 || proc == (void*)0x2 || proc == (void*)0x3 || proc == (void*)-1)
    {
        proc = (void*)GetProcAddress(g_wgl.library, name);
    }
    return proc;
}

static bool LoadGLESLibrary() {
    g_wgl.library = LoadLibraryA("opengl32.dll");
    if (!g_wgl.library)
        return false;

    wglCreateContext_ptr = (wglCreateContext_t*)GetProcAddress(g_wgl.library, "wglCreateContext");
    wglDeleteContext_ptr = (wglDeleteContext_t*)GetProcAddress(g_wgl.library, "wglDeleteContext");
    wglMakeCurrent_ptr = (wglMakeCurrent_t*)GetProcAddress(g_wgl.library, "wglMakeCurrent");
    wglGetProcAddress_ptr = (wglGetProcAddress_t*)GetProcAddress(g_wgl.library, "wglGetProcAddress");

    if (!wglCreateContext_ptr || !wglDeleteContext_ptr || !wglMakeCurrent_ptr || !wglGetProcAddress_ptr)
        return false;

    return true;
}

static void LoadGLESFunctions() {
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
    glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)GetGLProcAddress("glRenderbufferStorageMultisample");
    glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)GetGLProcAddress("glBlitFramebuffer");
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
    glVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)GetGLProcAddress("glVertexAttribIPointer");
    glViewport = (PFNGLVIEWPORTPROC)GetGLProcAddress("glViewport");
    glClipControl = (PFNGLCLIPCONTROLPROC)GetGLProcAddress("glClipControl");
    glGetIntegerv = (PFNGLGETINTEGERVPROC)GetGLProcAddress("glGetIntegerv");

    // WGL extensions
    wglCreateContextAttribsARB_ptr = (wglCreateContextAttribsARB_t*)GetGLProcAddress("wglCreateContextAttribsARB");
    wglChoosePixelFormatARB_ptr = (wglChoosePixelFormatARB_t*)GetGLProcAddress("wglChoosePixelFormatARB");
    wglSwapIntervalEXT_ptr = (wglSwapIntervalEXT_t*)GetGLProcAddress("wglSwapIntervalEXT");
}

static void UnloadGLESLibrary() {
    if (!g_wgl.library)
        return;

    FreeLibrary(g_wgl.library);
    g_wgl.library = nullptr;
}

void PlatformEndRender() {
    SwapBuffers(g_wgl.hdc);
}

// Shared function from gl_render.cpp
extern void CreateOffscreenTarget(OffscreenTarget& target, int width, int height, int samples);

void InitRenderDriver(const RendererTraits* traits, HWND hwnd) {
    g_gl = {};
    g_wgl = {};
    g_gl.traits = *traits;
    g_wgl.hwnd = hwnd;
    g_gl.depth_conversion_factor = 1.0f / (traits->max_depth - traits->min_depth);

    // Load OpenGL library
    if (!LoadGLESLibrary()) {
        Exit("Failed to load OpenGL library");
        return;
    }

    // Get device context
    g_wgl.hdc = GetDC(hwnd);
    if (!g_wgl.hdc) {
        Exit("Failed to get device context");
        return;
    }

    // Set pixel format
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixel_format = ChoosePixelFormat(g_wgl.hdc, &pfd);
    if (!pixel_format) {
        Exit("Failed to choose pixel format");
        return;
    }

    if (!SetPixelFormat(g_wgl.hdc, pixel_format, &pfd)) {
        Exit("Failed to set pixel format");
        return;
    }

    // Create temporary context to load WGL extensions
    HGLRC temp_context = wglCreateContext_ptr(g_wgl.hdc);
    if (!temp_context) {
        Exit("Failed to create temporary OpenGL context");
        return;
    }

    if (!wglMakeCurrent_ptr(g_wgl.hdc, temp_context)) {
        Exit("Failed to make temporary OpenGL context current");
        return;
    }

    // Load GL functions (needed for extension loading)
    LoadGLESFunctions();

    // Create modern OpenGL context if available
    if (wglCreateContextAttribsARB_ptr) {
        int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };

        g_wgl.hglrc = wglCreateContextAttribsARB_ptr(g_wgl.hdc, nullptr, attribs);
        if (g_wgl.hglrc) {
            wglMakeCurrent_ptr(g_wgl.hdc, nullptr);
            wglDeleteContext_ptr(temp_context);
            wglMakeCurrent_ptr(g_wgl.hdc, g_wgl.hglrc);
            // Reload functions with new context
            LoadGLESFunctions();
        } else {
            // Fall back to legacy context
            g_wgl.hglrc = temp_context;
        }
    } else {
        g_wgl.hglrc = temp_context;
    }

    // Enable vsync if available
    if (wglSwapIntervalEXT_ptr) {
        wglSwapIntervalEXT_ptr(1);
    }

    // Create default VAO (required in core profile)
    glGenVertexArrays(1, &g_gl.current_vao);
    glBindVertexArray(g_gl.current_vao);

    // Set up default GL state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    // Use standard GL conventions (Y-up, bottom-left origin)
    glFrontFace(GL_CCW);

    // Get screen size
    RECT rect;
    GetClientRect(hwnd, &rect);
    g_gl.screen_size = { rect.right - rect.left, rect.bottom - rect.top };
    g_gl.native_screen_size = g_gl.screen_size;

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

void ResizeRenderDriver(const Vec2Int& size) {
    if (size.x <= 0 || size.y <= 0)
        return;

    g_gl.screen_size = size;
    g_gl.native_screen_size = size;
    glViewport(0, 0, size.x, size.y);

    // Recreate offscreen target at new size (preserve MSAA sample count)
    int samples = g_gl.offscreen.samples;
    CreateOffscreenTarget(g_gl.offscreen, size.x, size.y, samples);
}

void PlatformSetRenderSize(Vec2Int logical_size, Vec2Int native_size) {
    if (logical_size.x <= 0 || logical_size.y <= 0)
        return;

    g_gl.native_screen_size = native_size;

    // Only recreate target if logical size changed
    if (g_gl.screen_size != logical_size) {
        g_gl.screen_size = logical_size;
        int samples = g_gl.offscreen.samples;
        CreateOffscreenTarget(g_gl.offscreen, logical_size.x, logical_size.y, samples);
    }
}

void ShutdownRenderDriver() {
    // Delete UBOs
    glDeleteBuffers(UNIFORM_BUFFER_COUNT, g_gl.ubos);

    if (g_gl.current_vao) {
        glDeleteVertexArrays(1, &g_gl.current_vao);
        g_gl.current_vao = 0;
    }

    if (g_wgl.hglrc) {
        wglMakeCurrent_ptr(nullptr, nullptr);
        wglDeleteContext_ptr(g_wgl.hglrc);
        g_wgl.hglrc = nullptr;
    }

    if (g_wgl.hdc && g_wgl.hwnd) {
        ReleaseDC(g_wgl.hwnd, g_wgl.hdc);
        g_wgl.hdc = nullptr;
    }

    UnloadGLESLibrary();
}
