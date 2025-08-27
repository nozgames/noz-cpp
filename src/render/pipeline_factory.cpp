//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define INITIAL_CACHE_SIZE 64

struct Pipeline
{
    SDL_GPUGraphicsPipeline* gpu_pipeline;
};

static Map g_cache = {};
static u64* g_cache_keys = nullptr;
static Pipeline* g_cache_pipelines = nullptr;
static SDL_GPUDevice* g_device = nullptr;
static SDL_Window* g_window = nullptr;

static uint64_t MakeKey(Shader* shader, bool msaa, bool shadow)
{
    struct {
        void* shader_ptr;
        bool msaa;
        bool shadow;
    } key_data = {shader, msaa, shadow};
    
    return Hash(&key_data, sizeof(key_data));
}

static uint32_t GetVertexStride(const SDL_GPUVertexAttribute* attributes, size_t attribute_count)
{
    if (attribute_count == 0) 
        return 0;

    // Calculate stride based on the last attribute's offset + size
    const SDL_GPUVertexAttribute* last_attr = &attributes[attribute_count - 1];
    uint32_t stride = last_attr->offset;

    // Add size based on attribute format
    switch (last_attr->format) 
    {
    case SDL_GPU_VERTEXELEMENTFORMAT_INT:
    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT:
    default:
        stride += 4;
        break;
    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2:
        stride += 8; // 2 * 4 bytes
        break;
    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3:
        stride += 12; // 3 * 4 bytes
        break;
    case SDL_GPU_VERTEXELEMENTFORMAT_INT4:
    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4:
        stride += 16; // 4 * 4 bytes
        break;
    }

    return stride;
}

static SDL_GPUGraphicsPipeline* CreateGPUPipeline(
    Shader* shader,
    const SDL_GPUVertexAttribute* attributes,
    size_t attribute_count,
    bool msaa,
    bool shadow)
{
    assert(g_window);
    assert(g_device);
    assert(shader);

    // Create pipeline directly using the shader's compiled shaders
    uint32_t vertexStride = GetVertexStride(attributes, attribute_count);

    SDL_GPUVertexBufferDescription vertex_buffer_desc = {};
    vertex_buffer_desc.pitch = vertexStride;
    vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexInputState vertex_input_state = {};
    vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
    vertex_input_state.num_vertex_buffers = 1;
    vertex_input_state.vertex_attributes = attributes;
    vertex_input_state.num_vertex_attributes = (uint32_t)attribute_count;

    SDL_GPUColorTargetDescription color_target = {};
    if (!shadow)
        color_target.format = SDL_GetGPUSwapchainTextureFormat(g_device, g_window);

    SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.vertex_shader = GetGPUVertexShader(shader);
    pipeline_create_info.fragment_shader = GetGPUFragmentShader(shader);
    pipeline_create_info.vertex_input_state = vertex_input_state;
    pipeline_create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_create_info.props = SDL_CreateProperties();

    SDL_SetStringProperty(pipeline_create_info.props, SDL_PROP_GPU_GRAPHICSPIPELINE_CREATE_NAME_STRING, GetGPUName(shader));

    // Set rasterizer state based on shader properties
    pipeline_create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeline_create_info.rasterizer_state.cull_mode = GetGPUCullMode(shader);
    pipeline_create_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    // Set multisample state based on current render target
    pipeline_create_info.multisample_state.sample_count = msaa ? SDL_GPU_SAMPLECOUNT_4 : SDL_GPU_SAMPLECOUNT_1;
    pipeline_create_info.multisample_state.sample_mask = 0;
    pipeline_create_info.multisample_state.enable_mask = false;

    // Get pipeline properties from shader
    bool depth_test = IsDepthTestEnabled(shader);
    bool depth_write = IsDepthWriteEnabled(shader);

    pipeline_create_info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    pipeline_create_info.depth_stencil_state.back_stencil_state.fail_op = SDL_GPU_STENCILOP_KEEP;
    pipeline_create_info.depth_stencil_state.back_stencil_state.pass_op = SDL_GPU_STENCILOP_KEEP;
    pipeline_create_info.depth_stencil_state.back_stencil_state.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
    pipeline_create_info.depth_stencil_state.back_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
    pipeline_create_info.depth_stencil_state.front_stencil_state.fail_op = SDL_GPU_STENCILOP_KEEP;
    pipeline_create_info.depth_stencil_state.front_stencil_state.pass_op = SDL_GPU_STENCILOP_KEEP;
    pipeline_create_info.depth_stencil_state.front_stencil_state.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
    pipeline_create_info.depth_stencil_state.front_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
    pipeline_create_info.depth_stencil_state.compare_mask = 0;
    pipeline_create_info.depth_stencil_state.write_mask = 0;
    pipeline_create_info.depth_stencil_state.enable_depth_test = depth_test;
    pipeline_create_info.depth_stencil_state.enable_depth_write = depth_write;
    pipeline_create_info.depth_stencil_state.enable_stencil_test = false;

    // Configure color targets and blend state (only for non-shadow shaders)
    if (!shadow) {
        SDL_GPUColorTargetBlendState blend_state = {};
        blend_state.enable_blend = IsBlendEnabled(shader);
        if (blend_state.enable_blend) {
            SDL_GPUBlendFactor src_blend = GetGPUSrcBlend(shader);
            SDL_GPUBlendFactor dst_blend = GetGPUDstBlend(shader);
            blend_state.enable_blend = true;
            blend_state.src_color_blendfactor = src_blend;
            blend_state.dst_color_blendfactor = dst_blend;
            blend_state.src_alpha_blendfactor = src_blend;
            blend_state.dst_alpha_blendfactor = dst_blend;
            blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
            blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        }

        // Update color target description with blend state
        color_target.blend_state = blend_state;

        pipeline_create_info.target_info.num_color_targets = 1;
        pipeline_create_info.target_info.color_target_descriptions = &color_target;
    } else {
        // Shadow shaders are depth-only, no color targets
        pipeline_create_info.target_info.num_color_targets = 0;
        pipeline_create_info.target_info.color_target_descriptions = nullptr;
    }

    // Set depth stencil target info to match the depth texture
    pipeline_create_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    pipeline_create_info.target_info.has_depth_stencil_target = true;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(g_device, &pipeline_create_info);
    SDL_DestroyProperties(pipeline_create_info.props);
    
    return pipeline;
}

SDL_GPUGraphicsPipeline* GetGPUPipeline(Shader* shader, bool msaa, bool shadow)
{
    assert(g_window);
    assert(g_device);
    assert(shader);

    auto key = MakeKey(shader, msaa, shadow);
    auto* pipeline = (Pipeline*)GetValue(g_cache, key);
    if (pipeline != nullptr)
        return pipeline->gpu_pipeline;

    // Create new pipeline
    SDL_GPUVertexAttribute attributes[] = {
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0},                 // position : POSITION (semantic 0)
        {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(float) * 3}, // uv0 : TEXCOORD1 (semantic 2)
        {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, sizeof(float) * 5}, // normal : TEXCOORD2 (semantic 1)
        {3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, sizeof(float) * 8}   // bone_index : TEXCOORD3 (semantic 3)
    };

    SDL_GPUGraphicsPipeline* gpu_pipeline = CreateGPUPipeline(shader, attributes, 4, msaa, shadow);
    if (!gpu_pipeline)
        return nullptr;

    pipeline = (Pipeline*)SetValue(g_cache, key, nullptr);
    if (!pipeline)
        ExitOutOfMemory("pipeline limit exceeded");

    pipeline->gpu_pipeline = gpu_pipeline;
    return pipeline->gpu_pipeline;
}

void InitPipelineFactory(RendererTraits* traits, SDL_Window* win, SDL_GPUDevice* dev)
{
    assert(!g_device);

    g_window = win;
    g_device = dev;
    g_cache_keys = (u64*)Alloc(nullptr, sizeof(u64) * traits->max_pipelines);
    g_cache_pipelines = (Pipeline*)Alloc(nullptr, sizeof(Pipeline) * traits->max_pipelines);
    Init(g_cache, g_cache_keys, g_cache_pipelines, traits->max_pipelines, sizeof(Pipeline));
}

void ShutdownPipelineFactory()
{
    assert(g_device);
    Free(nullptr, g_cache_keys);
    Free(nullptr, g_cache_pipelines);
    g_cache = {};
    g_window = nullptr;
    g_device = nullptr;
}
