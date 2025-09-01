//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

void DrawScreenCanvases();

static void ResetRenderState();
static void UpdateBackBuffer();
static void InitGammaPass();
static void InitShadowPass(const RendererTraits* traits);
static void RenderGammaPass();

struct Renderer
{
    SDL_GPUDevice* device;
    SDL_Window* window;
    SDL_GPUCommandBuffer* command_buffer;
    SDL_GPURenderPass* render_pass;
    mat4 view_projection;
    mat4 view;

    // gamma
    Mesh* gamma_mesh;
    Material* gamma_material;

    // Depth buffer support
    SDL_GPUTexture* depth_texture;
    int depth_width;
    int depth_height;

    // MSAA support
    SDL_GPUTexture* msaa_color_texture;
    SDL_GPUTexture* msaa_depth_texture;

    // Light view projection matrix for shadow mapping
    mat4 light_view;

    Texture* linear_back_buffer;
    SDL_GPUTexture* swap_chain_texture;
    SDL_GPUTexture* shadow_map;
    SDL_GPUSampler* shadow_sampler;
    Shader* shadow_shader;
    bool shadow_pass;
    bool msaa;
    SDL_GPUGraphicsPipeline* pipeline;
};

static Renderer g_renderer = {};

void InitShadowPass(const RendererTraits* traits)
{
    if (!traits->shadow_map_size)
        return;

    // Create shadow map using D32_FLOAT format for depth writing and sampling
    SDL_GPUTextureCreateInfo shadow_info = {};
    shadow_info.type = SDL_GPU_TEXTURETYPE_2D;
    shadow_info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT; // Depth format for depth-stencil target
    shadow_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    shadow_info.width = traits->shadow_map_size;
    shadow_info.height = traits->shadow_map_size;
    shadow_info.layer_count_or_depth = 1;
    shadow_info.num_levels = 1;
    shadow_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    shadow_info.props = SDL_CreateProperties();
    SDL_SetStringProperty(shadow_info.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, "ShadowMap");
    g_renderer.shadow_map = SDL_CreateGPUTexture(g_renderer.device, &shadow_info);
    SDL_DestroyProperties(shadow_info.props);
    if (!g_renderer.shadow_map)
        Exit(SDL_GetError());

    // Create shadow sampler with depth comparison
    SDL_GPUSamplerCreateInfo shadow_sampler_info = {};
    shadow_sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
    shadow_sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    shadow_sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    shadow_sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    shadow_sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    shadow_sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    shadow_sampler_info.enable_compare = true;
    shadow_sampler_info.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    g_renderer.shadow_sampler = SDL_CreateGPUSampler(g_renderer.device, &shadow_sampler_info);
    SDL_DestroyProperties(shadow_info.props);
    if (!g_renderer.shadow_sampler)
        Exit(SDL_GetError());
}

void BeginRenderFrame()
{
    ClearRenderCommands();
    UpdateBackBuffer();

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

        // Create MSAA color texture - use 16-bit float to match render target format
        SDL_GPUTextureCreateInfo msaaColorInfo = {};
        msaaColorInfo.type = SDL_GPU_TEXTURETYPE_2D;
        msaaColorInfo.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
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
}

void EndRenderFrame()
{
    assert(!g_renderer.render_pass);

    if (!g_renderer.command_buffer)
        return;

    RenderGammaPass();
    ExecuteRenderCommands(g_renderer.command_buffer);
    SDL_SubmitGPUCommandBuffer(g_renderer.command_buffer);

    g_renderer.command_buffer = nullptr;
    g_renderer.render_pass = nullptr;
}

SDL_GPURenderPass* BeginPassGPU(SDL_GPUTexture* target, bool clear, color_t clear_color)
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

SDL_GPURenderPass* BeginPassGPU(bool clear, color_t clear_color, bool msaa, Texture* target)
{
    assert(!g_renderer.render_pass);

    // TODO: handle msaa to a target texture
    SDL_GPUTexture* gpu_texture = target
        ? GetGPUTexture(target)
        : GetGPUTexture(g_renderer.linear_back_buffer);

    BeginPassGPU(gpu_texture, clear, clear_color);
    return g_renderer.render_pass;

#if 0
    SDL_GPUColorTargetInfo color_target = { 0 };
    SDL_GPUDepthStencilTargetInfo depth_target = { 0 };

    // Use MSAA textures for scene rendering with resolve to backbuffer
        if (false) // msaa && g_renderer.msaa_color_texture && g_renderer.msaa_depth_texture)
        {
            _msaa = true;
            color_target.texture = g_renderer.msaa_color_texture;
            color_target.clear_color = to_sdl(clear_color);
            color_target.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
            color_target.store_op = SDL_GPU_STOREOP_RESOLVE; // Resolve MSAA to backbuffer
            color_target.resolve_texture = gpu_texture; // Resolve target
            color_target.resolve_mip_level = 0;
            color_target.resolve_layer = 0;

            depth_target.texture = g_renderer.msaa_depth_texture;
            depth_target.clear_depth = 1.0f;
            depth_target.clear_stencil = 0;
            depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
            depth_target.store_op = SDL_GPU_STOREOP_DONT_CARE; // Don't need to store MSAA depth
        }
        // Standard rendering without MSAA
        else
        {
            _msaa = false;
            color_target.texture = gpu_texture;
            color_target.clear_color = to_sdl(clear_color);
            color_target.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
            color_target.store_op = SDL_GPU_STOREOP_STORE;

            depth_target.texture = g_renderer.depth_texture;
            depth_target.clear_depth = 1.0f;
            depth_target.clear_stencil = 0;
            depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
            depth_target.store_op = SDL_GPU_STOREOP_STORE;
        }

        g_renderer.render_pass = SDL_BeginGPURenderPass(g_renderer.command_buffer, &color_target, 1, &depth_target);
        assert(g_renderer.render_pass);

        ResetRenderState();

        return g_renderer.render_pass;
#endif
}

void EndRenderPassGPU()
{
    assert(g_renderer.render_pass);

    SDL_EndGPURenderPass(g_renderer.render_pass);
    g_renderer.render_pass = nullptr;
    g_renderer.shadow_pass = false;
    g_renderer.msaa = false;
}

void BindDefaultTextureGPU(int index)
{
    assert(g_renderer.device);
    BindTextureGPU(CoreAssets.textures.white, g_renderer.command_buffer, sampler_register_user0 + index);
}

void BindTextureGPU(Texture* texture, SDL_GPUCommandBuffer* cb, int index)
{
    if (g_renderer.shadow_pass)
        return;

    // Get the actual texture to bind (use default if none provided)
    Texture* actual_texture = texture ? texture : CoreAssets.textures.white;

    // Main pass: bind diffuse texture and shadow map
    SDL_GPUTextureSamplerBinding binding = {0};
    binding.sampler = GetGPUSampler(actual_texture);
    binding.texture = GetGPUTexture(actual_texture);
    SDL_BindGPUFragmentSamplers(g_renderer.render_pass, index, &binding, 1);
}

void BindShaderGPU(Shader* shader)
{
    assert(shader);

    SDL_GPUGraphicsPipeline* pipeline = GetGPUPipeline(
        g_renderer.shadow_pass
            ? g_renderer.shadow_shader
            : shader,
        g_renderer.msaa,
        g_renderer.shadow_pass);
    if (!pipeline)
        return;

    // Only bind if pipeline changed
    if (g_renderer.pipeline == pipeline)
        return;

    SDL_BindGPUGraphicsPipeline(g_renderer.render_pass, pipeline);
    g_renderer.pipeline = pipeline;
}

void BindTransformGPU(const mat4* transform)
{
    SDL_PushGPUVertexUniformData(
        g_renderer.command_buffer,
        vertex_register_object,
        transform,
        sizeof(mat4));
}

void BindBoneTransformsGPU(const mat4* bones, int count)
{
    assert(bones);
    assert(count > 0);

    SDL_PushGPUVertexUniformData(
        g_renderer.command_buffer,
        vertex_register_bone,
        bones,
        (Uint32)(count * sizeof(mat4)));
}

SDL_GPURenderPass* BeginUIPassGPU()
{
    assert(!g_renderer.render_pass);
    assert(g_renderer.command_buffer);

    return BeginPassGPU(g_renderer.swap_chain_texture, false, color_black);
}

SDL_GPURenderPass* BeginShadowPassGPU()
{
    assert(!g_renderer.render_pass);
    assert(g_renderer.command_buffer);

    // Start shadow pass using depth-only rendering
    SDL_GPUDepthStencilTargetInfo depth_info = {0};
    depth_info.texture = g_renderer.shadow_map;
    depth_info.clear_depth = 1.0f;
    depth_info.clear_stencil = 0;
    depth_info.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_info.store_op = SDL_GPU_STOREOP_STORE;

    g_renderer.render_pass = SDL_BeginGPURenderPass(g_renderer.command_buffer, nullptr, 0, &depth_info);
    g_renderer.shadow_pass = true;
    ResetRenderState();

    return g_renderer.render_pass;
}

static void ResetRenderState()
{
    // Reset all state tracking variables to force rebinding
    g_renderer.pipeline = nullptr;

    static mat4 identity = glm::identity<mat4>();
    BindBoneTransformsGPU(&identity, 1);

    for (int i = 0; i < (int)(sampler_register_count); i++)
        BindTextureGPU(CoreAssets.textures.white, g_renderer.command_buffer, i);
}

static void UpdateBackBuffer()
{
    // is the back buffer the correct size?
    assert(g_renderer.device);
    ivec2 size = GetScreenSize();
    if (g_renderer.linear_back_buffer && GetSize(g_renderer.linear_back_buffer) == size)
        return;

    g_renderer.linear_back_buffer =
        CreateTexture(nullptr, size.x, size.y, TEXTURE_FORMAT_RGBA16F, GetName("linear"));
}

static void InitGammaPass()
{
    MeshBuilder* builder = CreateMeshBuilder(nullptr, 4, 6);
    AddVertex(builder, vec3(-1.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(0.0f, 1.0f), 0);
    AddVertex(builder, vec3(1.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(1.0f, 1.0f), 0);
    AddVertex(builder, vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(1.0f, 0.0f), 0);
    AddVertex(builder, vec3(-1.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(0.0f, 0.0f), 0);
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 0, 2, 3);
    g_renderer.gamma_mesh = CreateMesh(nullptr, builder, GetName("gamma"));
    Destroy(builder);
}

SDL_GPURenderPass* BeginGammaPassGPU()
{
    if (!g_renderer.gamma_material)
        Exit("missing gamma shader");

    return BeginPassGPU(g_renderer.swap_chain_texture, false, color_black);
}

static void RenderGammaPass()
{
    SetTexture(g_renderer.gamma_material, g_renderer.linear_back_buffer, 0);
    BeginGammaPass();

    static auto identity = glm::identity<mat4>();
    BindCamera(identity, identity);
    BindTransform(identity);
    BindMaterial(g_renderer.gamma_material);
    DrawMesh(g_renderer.gamma_mesh);
    EndRenderPass();
}

void LoadRendererAssets(Allocator* allocator)
{
    CoreAssets.textures.white = CreateTexture(nullptr, &color32_white, 1, 1, TEXTURE_FORMAT_RGBA8, GetName("white"));
    g_renderer.gamma_material = CreateMaterial(ALLOCATOR_DEFAULT, CoreAssets.shaders.gamma);
}

void InitRenderer(RendererTraits* traits, SDL_Window* window)
{
    g_renderer.window = window;

    // Create GPU device
    g_renderer.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
    if (!g_renderer.device)
        Exit(SDL_GetError());

    if (!SDL_ClaimWindowForGPUDevice(g_renderer.device, window))
    {
        SDL_DestroyGPUDevice(g_renderer.device);
        Exit(SDL_GetError());
    }

    InitTexture(traits, g_renderer.device);
    InitShader(traits, g_renderer.device);
    InitFont(traits, g_renderer.device);
    InitMesh(traits, g_renderer.device);
    InitRenderBuffer(traits);
    InitSamplerFactory(traits, g_renderer.device);
    InitPipelineFactory(traits, window, g_renderer.device);
    InitGammaPass();
    InitShadowPass(traits);
}

void ShutdownRenderer()
{
    assert(g_renderer.device);

    ShutdownPipelineFactory();
    ShutdownSamplerFactory();
    ShutdownRenderBuffer();
    ShutdownMesh();
    ShutdownFont();
    ShutdownShader();
    ShutdownTexture();

    // g_renderer.gamma_mesh = nullptr; // TODO: implement proper cleanup

    if (g_renderer.depth_texture)
        SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.depth_texture);

    if (g_renderer.msaa_color_texture)
        SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.msaa_color_texture);

    if (g_renderer.msaa_depth_texture)
        SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.msaa_depth_texture);

    // Release shadow resources
    if (g_renderer.shadow_map)
        SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.shadow_map);

    if (g_renderer.shadow_sampler)
        SDL_ReleaseGPUSampler(g_renderer.device, g_renderer.shadow_sampler);

    SDL_DestroyGPUDevice(g_renderer.device);

    g_renderer = {};
}
