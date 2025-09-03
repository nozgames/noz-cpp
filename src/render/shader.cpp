//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct ShaderImpl : Shader
{
    // SDL_GPUShader* vertex;
    // SDL_GPUShader* fragment;
    int vertex_uniform_count;
    int fragment_uniform_count;
    int sampler_count;
    shader_flags_t flags;
    // SDL_GPUBlendFactor src_blend;
    // SDL_GPUBlendFactor dst_blend;
    // SDL_GPUCullMode cull;
    const Name* name;
    size_t uniform_data_size;
    ShaderUniformBuffer* uniforms;
};

//static SDL_GPUDevice* g_device = nullptr;

// todo: destructor
#if 0

static void shader_destroy_impl(ShaderImpl* impl)
{
    assert(impl);
    if (impl->vertex)
    {
        SDL_ReleaseGPUShader(g_device, impl->vertex);
        impl->vertex = nullptr;
    }

    if (impl->fragment)
    {
        SDL_ReleaseGPUShader(g_device, impl->fragment);
        impl->fragment = nullptr;
    }    
}
#endif

Asset* LoadShader(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name)
{
    assert(stream);
    assert(header);
    assert(name);

    auto vertex_uniform_count = ReadI32(stream);
    auto fragment_uniform_count = ReadI32(stream);
    auto sampler_count = ReadI32(stream);
    size_t uniform_count = vertex_uniform_count + fragment_uniform_count;

    Shader* shader = (Shader*)Alloc(allocator, sizeof(ShaderImpl) + sizeof(ShaderUniformBuffer) * uniform_count);
    if (!shader)
        return nullptr;

#if 0
    ShaderImpl* impl = static_cast<ShaderImpl*>(shader);
    impl->vertex = nullptr;
    impl->fragment = nullptr;
    impl->vertex_uniform_count = vertex_uniform_count;
    impl->fragment_uniform_count = fragment_uniform_count;
    impl->sampler_count = sampler_count;
    impl->flags = shader_flags_none;
    impl->src_blend = SDL_GPU_BLENDFACTOR_ONE;
    impl->dst_blend = SDL_GPU_BLENDFACTOR_ZERO;
    impl->cull = SDL_GPU_CULLMODE_NONE;
    impl->name = name;
    impl->uniforms = (ShaderUniformBuffer*)(impl + 1);

    // Read the vertex shader
    auto vertex_bytecode_length = ReadU32(stream);
    auto* vertex_bytecode = (u8*)Alloc(ALLOCATOR_SCRATCH, vertex_bytecode_length);
    if (!vertex_bytecode) 
        return nullptr;
    ReadBytes(stream, vertex_bytecode, vertex_bytecode_length);

    // Read the fragment shader
    auto fragment_bytecode_length = ReadU32(stream);
    auto* fragment_bytecode = (u8*)Alloc(ALLOCATOR_SCRATCH, fragment_bytecode_length);
    if (!fragment_bytecode) 
    {
        Free(vertex_bytecode);
        return nullptr;
    }
    ReadBytes(stream, fragment_bytecode, fragment_bytecode_length);

    impl->flags = (shader_flags_t)ReadU8(stream);
    impl->src_blend = (SDL_GPUBlendFactor)ReadU32(stream);
    impl->dst_blend = (SDL_GPUBlendFactor)ReadU32(stream);
    impl->cull = (SDL_GPUCullMode)ReadU32(stream);

    ReadBytes(stream, impl->uniforms, uniform_count * sizeof(ShaderUniformBuffer));

    if (uniform_count > 0)
        impl->uniform_data_size = impl->uniforms[uniform_count-1].offset + impl->uniforms[uniform_count-1].size;

    // Note: stream destruction handled by caller

    // Create fragment shader
    SDL_GPUShaderCreateInfo fragment_create_info = {0};
    fragment_create_info.code = fragment_bytecode;
    fragment_create_info.code_size = fragment_bytecode_length;
    fragment_create_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragment_create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    fragment_create_info.entrypoint = "ps";
    fragment_create_info.num_samplers = impl->sampler_count + (u32)sampler_register_user0;
    fragment_create_info.num_storage_textures = 0;
    fragment_create_info.num_storage_buffers = 0;
    fragment_create_info.num_uniform_buffers = impl->fragment_uniform_count + (u32)fragment_register_user0;
    fragment_create_info.props = SDL_CreateProperties();

    SDL_SetStringProperty(fragment_create_info.props, SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, name->value);
    impl->fragment = SDL_CreateGPUShader(g_device, &fragment_create_info);
    SDL_DestroyProperties(fragment_create_info.props);

    if (!impl->fragment)
    {
        Free(vertex_bytecode);
        Free(fragment_bytecode);
        return nullptr;
    }

    // Create vertex shader
    SDL_GPUShaderCreateInfo vertex_create_info = {0};
    vertex_create_info.code = vertex_bytecode;
    vertex_create_info.code_size = vertex_bytecode_length;
    vertex_create_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertex_create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    vertex_create_info.entrypoint = "vs";
    vertex_create_info.num_samplers = 0;
    vertex_create_info.num_storage_textures = 0;
    vertex_create_info.num_storage_buffers = 0;
    vertex_create_info.num_uniform_buffers = impl->vertex_uniform_count + (u32)vertex_register_user0;
    vertex_create_info.props = SDL_CreateProperties();

    SDL_SetStringProperty(vertex_create_info.props, SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, name->value);
    impl->vertex = SDL_CreateGPUShader(g_device, &vertex_create_info);
    SDL_DestroyProperties(vertex_create_info.props);

    Free(vertex_bytecode);
    Free(fragment_bytecode);

    if (!impl->vertex)
        return nullptr;

#endif
    return shader;
}

#if 0
SDL_GPUShader* GetGPUVertexShader(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->vertex;
}

SDL_GPUShader* GetGPUFragmentShader(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->fragment;
}

SDL_GPUCullMode GetGPUCullMode(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->cull;
}

bool IsBlendEnabled(Shader* shader)
{
    return (static_cast<ShaderImpl*>(shader)->flags & shader_flags_blend) != 0;
}

SDL_GPUBlendFactor GetGPUSrcBlend(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->src_blend;
}

SDL_GPUBlendFactor GetGPUDstBlend(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->dst_blend;
}

bool IsDepthTestEnabled(Shader* shader)
{
    return (static_cast<ShaderImpl*>(shader)->flags & shader_flags_depth_test) != 0;
}

bool IsDepthWriteEnabled(Shader* shader)
{
    return (static_cast<ShaderImpl*>(shader)->flags & shader_flags_depth_write) != 0;
}

int GetVertexUniformCount(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->vertex_uniform_count;
}

int GetFragmentUniformCount(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->fragment_uniform_count;
}

int GetSamplerCount(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->sampler_count;
}

const char* GetGPUName(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->name->value;
}

size_t GetUniformDataSize(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->uniform_data_size;
}

ShaderUniformBuffer GetVertexUniformBuffer(Shader* shader, int index)
{
    assert(index >=0 && index < static_cast<ShaderImpl*>(shader)->vertex_uniform_count);
    return static_cast<ShaderImpl*>(shader)->uniforms[index];
}

ShaderUniformBuffer GetFragmentUniformBuffer(Shader* shader, int index)
{
    auto impl = static_cast<ShaderImpl*>(shader);
    assert(index >=0 && index < impl->fragment_uniform_count);
    return impl->uniforms[index + impl->vertex_uniform_count];
}

void PushUniformDataGPU(Shader* shader, SDL_GPUCommandBuffer* cb, u8* data)
{
    auto impl = static_cast<ShaderImpl*>(shader);

    // vertex
    for (int i=0, c=impl->vertex_uniform_count; i<c; i++)
    {
        auto [size, offset] = impl->uniforms[i];
        SDL_PushGPUVertexUniformData(
            cb,
            vertex_register_user0 + i,
            data + offset,
            size);
    }

    // fragment
    for (int i=0, c=impl->fragment_uniform_count; i<c; i++)
    {
        auto [size, offset] = impl->uniforms[i + impl->vertex_uniform_count];
        SDL_PushGPUFragmentUniformData(
            cb,
            fragment_register_user0 + i,
            data + offset,
            size);
    }
}

void InitShader(RendererTraits* traits, SDL_GPUDevice* device)
{
    g_device = device;
}

void ShutdownShader()
{
    assert(g_device);
    g_device = nullptr;
}
#endif