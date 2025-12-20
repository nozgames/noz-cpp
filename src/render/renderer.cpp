//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

static Mesh* GetFullscreenQuad();
extern void BindMaterialInternal(Material* material);
extern void RenderMesh(Mesh* mesh);
extern void InitRenderBuffer(const RendererTraits* traits);
extern void ShutdownRenderDriver();
extern void ShutdownRenderBuffer();
extern void ExecuteRenderCommands();
extern void ClearRenderCommands();
extern void DrawUI();

struct Renderer {
    RendererTraits traits;
    Mesh* fullscreen_quad;
    Material* ui_composite_material;
    Material* postprocess_material;
    bool postprocess_enabled;
    PostProcessParams postprocess_params = {0.0f, 0.0f};
};

static Renderer g_renderer = {};
Texture* TEXTURE_WHITE = nullptr;

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

void SetPostProcessMaterial(Material* material) {
    g_renderer.postprocess_material = material;
}

Material* GetPostProcessMaterial() {
    return g_renderer.postprocess_material;
}

void SetUICompositeMaterial(Material* material) {
    g_renderer.ui_composite_material = material;
}

Material* GetUICompositeMaterial() {
    return g_renderer.ui_composite_material;
}

void PostProcPass() {
    // Disabled for now - going straight from Scene -> UI -> Composite
#if 0
    PlatformTransitionSceneTexture();

    if (!g_renderer.postprocess_material)
        return;

    PlatformBeginPostProcPass();

    Mat3 identity = MAT3_IDENTITY;
    PlatformBindCamera(identity);
    PlatformBindTransform(identity, 0.0f, 1.0f);
    PlatformBindColor(COLOR_WHITE, VEC2_ZERO, COLOR_TRANSPARENT);
    BindMaterialInternal(g_renderer.postprocess_material);
    PlatformBindSceneTexture();
    RenderMesh(GetFullscreenQuad());

    PlatformEndPostProcPass();
#endif
}

void UIPass() {
    if (!g_renderer.ui_composite_material)
        return;

    // Draw UI to same offscreen target as scene (no separate UI target)
    DrawUI();
    ExecuteRenderCommands();
}

void BeginRender(Color clear_color) {
    ClearRenderCommands();
    PlatformBeginRender();
    PlatformBeginScenePass(clear_color);
}

void CompositePass() {
    PlatformBeginCompositePass();

    PlatformBindCamera(MAT3_IDENTITY);
    PlatformBindTransform(MAT3_IDENTITY, 0.0f, 1.0f);
    PlatformBindColor(COLOR_WHITE, VEC2_ZERO, COLOR_TRANSPARENT);
    if (g_renderer.ui_composite_material)
        BindMaterialInternal(g_renderer.ui_composite_material);
    // Rotation is handled in PlatformBindSceneTexture if needed
    PlatformBindSceneTexture();
    RenderMesh(GetFullscreenQuad());
    PlatformEndCompositePass();
}

void EndRender() {
    ExecuteRenderCommands();  // Flush scene commands
    UIPass();                 // Render UI to same target
    PlatformEndScenePass();   // Resolve MSAA (scene + UI combined)
    CompositePass();          // Present to screen
    PlatformEndRender();
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
