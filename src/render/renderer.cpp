//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

extern void BeginRenderPass();
extern void EndRenderPass();
extern void BeginRenderPass(Color clear_color);
extern void BeginShadowPass(Mat4 light_view, Mat4 light_projection);
extern void InitRenderBuffer(const RendererTraits* traits);
extern void ShutdownVulkan();
extern void ShutdownRenderBuffer();
extern void ExecuteRenderCommands();
extern void ClearRenderCommands();
extern platform::Pipeline* GetPipeline(Shader* shader);

static void ResetRenderState();

struct Renderer {
    RendererTraits traits;
    platform::Pipeline* pipeline;
};

static Renderer g_renderer = {};
Texture* TEXTURE_WHITE = nullptr;

void BeginRenderFrame(Color clear_color) {
    platform::BeginRenderFrame();
    ClearRenderCommands();
    ResetRenderState();
    BeginRenderPass(clear_color);
}

void EndRenderFrame() {
    EndRenderPass();
    ExecuteRenderCommands();
    platform::EndRenderFrame();
}

static void ResetRenderState() {
    g_renderer.pipeline = nullptr;
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
