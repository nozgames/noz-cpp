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
    platform::BeginRenderFrame();
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
        BeginPostProcessPass();
        DrawPostProcessQuad(g_renderer.postprocess_material);
        EndPostProcessPass();
    }

    ClearRenderCommands();
    ResetRenderState();
    platform::BeginUIPass();

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

    platform::EndSwapchainPass();

    if (g_renderer.ui_composite_material) {
        ClearRenderCommands();
        ResetRenderState();

        Mat3 identity = MAT3_IDENTITY;
        platform::BindCamera(identity);
        platform::BindTransform(identity, 0.0f, 1.0f);
        platform::BindColor(COLOR_WHITE, VEC2_ZERO, COLOR_TRANSPARENT);

        BindMaterialInternal(g_renderer.ui_composite_material);
        platform::BindUIOffscreenTexture();

        RenderMesh(GetFullscreenQuad());

        ExecuteRenderCommands();
    }

    platform::EndRenderPass();
    g_renderer.ui_pass_active = false;
}

void EndRenderFrame() {
    BeginUIPass();
    DrawUI();
    EndUIPass();

    platform::EndRenderFrame();
}

static void ResetRenderState() {
    BindColor(COLOR_WHITE);
    BindLight(VEC3_FORWARD, COLOR_WHITE, COLOR_BLACK);
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
    platform::SetPostProcessEnabled(enabled);
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
        // Create fullscreen quad covering NDC space (-1,-1) to (1,1)
        Vec3 positions[] = {
            {-1.0f, -1.0f, 0.0f},
            { 1.0f, -1.0f, 0.0f},
            { 1.0f,  1.0f, 0.0f},
            {-1.0f,  1.0f, 0.0f}
        };
        Vec2 uvs[] = {
            {0.0f, 0.0f},  // Bottom-left
            {1.0f, 0.0f},  // Bottom-right
            {1.0f, 1.0f},  // Top-right
            {0.0f, 1.0f}   // Top-left
        };
        u16 indices[] = {0, 1, 2, 0, 2, 3};

        g_renderer.fullscreen_quad = CreateMesh(nullptr, 4, positions, uvs, 6, indices, GetName("fullscreen_quad"));
    }
    return g_renderer.fullscreen_quad;
}

void BeginPostProcessPass() {
    platform::BeginPostProcessPass();
}

void EndPostProcessPass() {
    platform::EndPostProcessPass();
}

void DrawPostProcessQuad(Material* material) {
    // Set up identity transform for fullscreen quad (in NDC space, no camera transform needed)
    Mat3 identity = MAT3_IDENTITY;
    platform::BindCamera(identity);  // Identity camera for NDC space
    platform::BindTransform(identity, 0.0f, 1.0f);
    platform::BindColor(COLOR_WHITE, VEC2_ZERO, COLOR_TRANSPARENT);

    // Bind material first (sets up shader and any material-specific uniforms)
    BindMaterialInternal(material);

    // Bind the offscreen texture AFTER material, so it overrides any texture slot 0
    platform::BindOffscreenTexture();

    RenderMesh(GetFullscreenQuad());
}
