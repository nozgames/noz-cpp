//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

extern void DrawUI();
extern void DrawVfx();
extern void BeginRenderPass();
extern void EndRenderPass();
extern void ClearRenderCommands();
extern void BeginRenderPass(Color clear_color);
extern void BeginShadowPass(Mat4 light_view, Mat4 light_projection);
extern void InitRenderBuffer(const RendererTraits* traits);
extern void ShutdownVulkan();
extern void ShutdownRenderBuffer();
extern void ExecuteRenderCommands();
extern platform::Pipeline* GetPipeline(Shader* shader);

static void ResetRenderState();

struct Renderer
{
    RendererTraits traits;
    platform::Pipeline* pipeline;
};

static Renderer g_renderer = {};

void BeginRenderFrame(Color clear_color)
{
    platform::BeginRenderFrame();
    ClearRenderCommands();
    ResetRenderState();
    BeginRenderPass(clear_color);

#if 0

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(g_renderer.device);
    //SDL_GPUTexture* backbuffer = nullptr;
    Uint32 width, height;
    SDL_WaitAndAcquireGPUSwapchainTexture(cmd, g_renderer.window, &g_renderer.swap_chain_texture, &width, &height);
    if (!g_renderer.swap_chain_texture)
        return;

    if (width == 0 || height == 0)
    {
        int windowWidth;
        int windowHeight;
        SDL_GetWindowSize(g_renderer.window, &windowWidth, &windowHeight);

        if (windowWidth > 0 && windowHeight > 0)
        {
            width = (Uint32)windowWidth;
            height = (Uint32)windowHeight;
        }
        else
        {
            width = 800;
            height = 600;
        }
    }

    // Create depth texture if it doesn't exist or if size changed
    if (!g_renderer.depth_texture || g_renderer.depth_width != (int)width || g_renderer.depth_height != (int)height)
    {
        if (g_renderer.depth_texture)
        {
            SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.depth_texture);
            g_renderer.depth_texture = nullptr;
        }

        // Create property group for D3D12 clear depth value to match our clear value
        SDL_PropertiesID depthProps = SDL_CreateProperties();
        SDL_SetFloatProperty(depthProps, SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT, 1.0f);

        SDL_GPUTextureCreateInfo depthInfo = {};
        depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
        depthInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        depthInfo.width = width;
        depthInfo.height = height;
        depthInfo.layer_count_or_depth = 1;
        depthInfo.num_levels = 1;
        depthInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
        depthInfo.props = depthProps; // Use the D3D12 properties

        g_renderer.depth_texture = SDL_CreateGPUTexture(g_renderer.device, &depthInfo);
        if (!g_renderer.depth_texture) {
            // Handle error - for now just assert
            assert(g_renderer.depth_texture);
        }

        SDL_DestroyProperties(depthProps);

        // Create MSAA textures
        if (g_renderer.msaa_color_texture)
        {
            SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.msaa_color_texture);
            g_renderer.msaa_color_texture = nullptr;
        }
        if (g_renderer.msaa_depth_texture)
        {
            SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.msaa_depth_texture);
            g_renderer.msaa_depth_texture = nullptr;
        }

        // Create MSAA color texture - use swap chain format for compatibility
        SDL_GPUTextureCreateInfo msaaColorInfo = {};
        msaaColorInfo.type = SDL_GPU_TEXTURETYPE_2D;
        msaaColorInfo.format = SDL_GetGPUSwapchainTextureFormat(g_renderer.device, g_renderer.window);
        msaaColorInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        msaaColorInfo.width = width;
        msaaColorInfo.height = height;
        msaaColorInfo.layer_count_or_depth = 1;
        msaaColorInfo.num_levels = 1;
        msaaColorInfo.sample_count = SDL_GPU_SAMPLECOUNT_4; // 4x MSAA
        msaaColorInfo.props = SDL_CreateProperties();
        SDL_SetStringProperty(msaaColorInfo.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, "MSAAColor");

        g_renderer.msaa_color_texture = SDL_CreateGPUTexture(g_renderer.device, &msaaColorInfo);

        SDL_DestroyProperties(msaaColorInfo.props);

        // Create MSAA depth texture
        SDL_PropertiesID msaaDepthProps = SDL_CreateProperties();
        SDL_SetFloatProperty(msaaDepthProps, SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT, 1.0f);

        SDL_GPUTextureCreateInfo msaaDepthInfo = {};
        msaaDepthInfo.type = SDL_GPU_TEXTURETYPE_2D;
        msaaDepthInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        msaaDepthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        msaaDepthInfo.width = width;
        msaaDepthInfo.height = height;
        msaaDepthInfo.layer_count_or_depth = 1;
        msaaDepthInfo.num_levels = 1;
        msaaDepthInfo.sample_count = SDL_GPU_SAMPLECOUNT_4; // 4x MSAA
        msaaDepthInfo.props = msaaDepthProps;

        g_renderer.msaa_depth_texture = SDL_CreateGPUTexture(g_renderer.device, &msaaDepthInfo);

        g_renderer.depth_width = (int)width;
        g_renderer.depth_height = (int)height;
    }

    g_renderer.command_buffer = cmd;

#endif

}

void EndRenderFrame()
{
    EndRenderPass();
    ExecuteRenderCommands();
    platform::EndRenderFrame();
}

#if 0
SDL_GPURenderPass* BeginPassGPU(SDL_GPUTexture* target, bool clear, Color clear_color)
{
    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture = target;
    color_target.clear_color = ColorToSDL(clear_color);
    color_target.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
    color_target.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depth_target = {};
    depth_target.texture = g_renderer.depth_texture;
    depth_target.clear_depth = 1.0f;
    depth_target.clear_stencil = 0;
    depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_target.store_op = SDL_GPU_STOREOP_STORE;

    g_renderer.render_pass = SDL_BeginGPURenderPass(g_renderer.command_buffer, &color_target, 1, &depth_target);
    assert(g_renderer.render_pass);

    ResetRenderState();

    return g_renderer.render_pass;
}

SDL_GPURenderPass* BeginPassGPU(bool clear, Color clear_color, bool msaa, Texture* target)
{
    assert(!g_renderer.render_pass);

    SDL_GPUTexture* gpu_texture = target
        ? GetGPUTexture(target)
        : g_renderer.swap_chain_texture;

    // Use MSAA if requested and MSAA textures are available
    if (msaa && g_renderer.msaa_color_texture && g_renderer.msaa_depth_texture && !target)
    {
        g_renderer.msaa = true;
        
        SDL_GPUColorTargetInfo color_target = {};
        color_target.texture = g_renderer.msaa_color_texture;
        color_target.clear_color = ColorToSDL(clear_color);
        color_target.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
        color_target.store_op = SDL_GPU_STOREOP_RESOLVE; // Resolve MSAA to backbuffer
        color_target.resolve_texture = gpu_texture; // Resolve target
        color_target.resolve_mip_level = 0;
        color_target.resolve_layer = 0;

        SDL_GPUDepthStencilTargetInfo depth_target = {};
        depth_target.texture = g_renderer.msaa_depth_texture;
        depth_target.clear_depth = 1.0f;
        depth_target.clear_stencil = 0;
        depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
        depth_target.store_op = SDL_GPU_STOREOP_DONT_CARE; // Don't need to store MSAA depth

        g_renderer.render_pass = SDL_BeginGPURenderPass(g_renderer.command_buffer, &color_target, 1, &depth_target);
        assert(g_renderer.render_pass);

        ResetRenderState();
        return g_renderer.render_pass;
    }
    
    // Standard rendering without MSAA
    g_renderer.msaa = false;
    BeginPassGPU(gpu_texture, clear, clear_color);
    return g_renderer.render_pass;

}
#endif

void BindDefaultTextureGPU(int index)
{
#if 0
    assert(g_renderer.device);
    BindTextureGPU(g_core_assets.textures.white, g_renderer.command_buffer, SAMPLER_REGISTER_TEX0 + index);
#endif
}

#if 0
void BindTextureGPU(Texture* texture, SDL_GPUCommandBuffer* cb, int index)
{
    if (g_renderer.shadow_pass)
        return;

    // Get the actual texture to bind (use default if none provided)
    Texture* actual_texture = texture ? texture : g_core_assets.textures.white;

    // Main pass: bind diffuse texture and shadow map
    SDL_GPUTextureSamplerBinding binding = {0};
    binding.sampler = GetGPUSampler(actual_texture);
    binding.texture = GetGPUTexture(actual_texture);
    SDL_BindGPUFragmentSamplers(g_renderer.render_pass, index, &binding, 1);
}

#endif

static void ResetRenderState()
{
    // Reset all state tracking variables to force rebinding
    g_renderer.pipeline = nullptr;

#if 0
    static Mat4 identity = MAT4_IDENTITY;
    BindBoneTransformsGPU(&identity, 1);

    for (int i = 0; i < (int)(sampler_register_count); i++)
        BindTextureGPU(g_core_assets.textures.white, g_renderer.command_buffer, i);
#endif
}

void LoadRendererAssets(Allocator* allocator)
{
    g_core_assets.textures.white = CreateTexture(nullptr, &color32_white, 1, 1, TEXTURE_FORMAT_RGBA8, GetName("white"));
}

void InitRenderer(const RendererTraits* traits)
{
    InitRenderBuffer(traits);
}

void ShutdownRenderer()
{
    ShutdownRenderBuffer();

    g_renderer = {};
}
