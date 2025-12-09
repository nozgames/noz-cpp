//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

static Mesh* GetFullscreenQuad();
extern void BindMaterialInternal(Material* material);
extern void RenderMesh(Mesh* mesh);
extern void BeginRenderPass();
extern void EndRenderPass();
extern void BeginRenderPass(Color clear_color);
extern void BeginShadowPass(Mat4 light_view, Mat4 light_projection);
extern void InitRenderBuffer(const RendererTraits* traits);
extern void ShutdownVulkan();
extern void ShutdownRenderBuffer();
extern void ExecuteRenderCommands();
extern void ClearRenderCommands();
extern void DrawUI();
static void ResetRenderState();

struct Renderer {
    RendererTraits traits;
    Mesh* fullscreen_quad = nullptr;
    Material* ui_composite_material;
    bool ui_pass_active = false;
    Material* postprocess_material;
    bool postprocess_enabled = false;
    PostProcessParams postprocess_params = {0.0f, 0.0f};
};

static Renderer g_renderer = {};
Texture* TEXTURE_WHITE = nullptr;

void BeginRenderFrame(Color clear_color) {
    PlatformBeginRenderFrame();
    ClearRenderCommands();
    ResetRenderState();
    BeginRenderPass(clear_color);
}

void SetPostProcessMaterial(Material* material) {
    g_renderer.postprocess_material = material;
}

Material* GetPostProcessMaterial() {
    return g_renderer.postprocess_material;
}

void BeginUIPass() {
    EndRenderPass();
    ExecuteRenderCommands();

    if (IsPostProcessEnabled() && g_renderer.postprocess_material) {
        PlatformBeginPostProcess();
        DrawPostProcessQuad(g_renderer.postprocess_material);
        EndPostProcessPass();
    }

    ClearRenderCommands();
    ResetRenderState();
    PlatformBeginUI();

    g_renderer.ui_pass_active = true;
}

void SetUICompositeMaterial(Material* material) {
    g_renderer.ui_composite_material = material;
}

Material* GetUICompositeMaterial() {
    return g_renderer.ui_composite_material;
}

static void EndUIPass() {
    ExecuteRenderCommands();

    PlatformEndSwapChain();

    if (g_renderer.ui_composite_material) {
        ClearRenderCommands();
        ResetRenderState();

        Mat3 identity = MAT3_IDENTITY;
        PlatformBindCamera(identity);
        PlatformBindTransform(identity, 0.0f, 1.0f);
        PlatformBindColor(COLOR_WHITE, VEC2_ZERO, COLOR_TRANSPARENT);

        BindMaterialInternal(g_renderer.ui_composite_material);
        PlatformBindUITexture();

        RenderMesh(GetFullscreenQuad());

        ExecuteRenderCommands();
    }

    PlatformEndRenderPass();
    g_renderer.ui_pass_active = false;
}

void EndRenderFrame() {
    BeginUIPass();
    DrawUI();
    EndUIPass();

    PlatformEndRenderFrame();
}

static void ResetRenderState() {
    BindColor(COLOR_WHITE);
}

void LoadRendererAssets(Allocator* allocator) {
    (void)allocator;

    TEXTURE_WHITE = CreateTexture(nullptr, &color32_white, 1, 1, TEXTURE_FORMAT_RGBA8, GetName("white"));
}

void InitRenderer(const RendererTraits* traits) {
    InitRenderBuffer(traits);
}

void ShutdownRenderer() {
    ShutdownRenderBuffer();
    g_renderer = {};
}

void EnablePostProcess(bool enabled) {
    g_renderer.postprocess_enabled = enabled;
    PlatformEnablePostProcess(enabled);
}

bool IsPostProcessEnabled() {
    return g_renderer.postprocess_enabled;
}

void SetPostProcessParams(const PostProcessParams& params) {
    g_renderer.postprocess_params = params;
}

const PostProcessParams& GetPostProcessParams() {
    return g_renderer.postprocess_params;
}

static Mesh* GetFullscreenQuad() {
    if (!g_renderer.fullscreen_quad) {
        static MeshVertex vertices[] = {
            { {-1.0f, -1.0f}, 0.0f, {0.0f, 0.0f} },
            { { 1.0f, -1.0f}, 0.0f, {1.0f, 0.0f} },
            { { 1.0f,  1.0f}, 0.0f, {1.0f, 1.0f} },
            { {-1.0f,  1.0f}, 0.0f, {0.0f, 1.0f} }
        };
        static u16 indices[] = {0, 1, 2, 0, 2, 3};
        g_renderer.fullscreen_quad = CreateMesh(nullptr, 4, vertices, 6, indices, GetName("fullscreen_quad"));
    }
    return g_renderer.fullscreen_quad;
}

void BeginPostProcessPass() {
    PlatformBeginPostProcess();
}

void EndPostProcessPass() {
    PlatformEndPostProcess();
}

void DrawPostProcessQuad(Material* material) {
    Mat3 identity = MAT3_IDENTITY;
    PlatformBindCamera(identity);
    PlatformBindTransform(identity, 0.0f, 1.0f);
    PlatformBindColor(COLOR_WHITE, VEC2_ZERO, COLOR_TRANSPARENT);

    BindMaterialInternal(material);

    PlatformBindOffscreenTexture();

    RenderMesh(GetFullscreenQuad());
}
