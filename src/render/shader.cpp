//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

struct ShaderImpl : Shader
{
    platform::ShaderModule* vertex;
    platform::ShaderModule* fragment;
    int vertex_uniform_count;
    int fragment_uniform_count;
    int sampler_count;
    shader_flags_t flags;
    int src_blend;
    int dst_blend;
    int cull;
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
    ShaderImpl* impl = static_cast<ShaderImpl*>(shader);
    impl->vertex = nullptr;
    impl->fragment = nullptr;
    impl->vertex_uniform_count = vertex_uniform_count;
    impl->fragment_uniform_count = fragment_uniform_count;
    impl->sampler_count = sampler_count;
    impl->flags = shader_flags_none;
    impl->src_blend = 1; // One
    impl->dst_blend = 0; // Zero
    impl->cull = 0; // None
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
    impl->src_blend = ReadU32(stream);
    impl->dst_blend = ReadU32(stream);
    impl->cull = ReadU32(stream);

    ReadBytes(stream, impl->uniforms, uniform_count * sizeof(ShaderUniformBuffer));

    if (uniform_count > 0)
        impl->uniform_data_size = impl->uniforms[uniform_count-1].offset + impl->uniforms[uniform_count-1].size;

    // Note: stream destruction handled by caller

    // Create fragment shader using platform API
    impl->fragment = platform::CreateShaderModule(fragment_bytecode, fragment_bytecode_length);
    if (!impl->fragment)
    {
        Free(vertex_bytecode);
        Free(fragment_bytecode);
        return nullptr;
    }

    // Create vertex shader using platform API
    impl->vertex = platform::CreateShaderModule(vertex_bytecode, vertex_bytecode_length);
    
    Free(vertex_bytecode);
    Free(fragment_bytecode);

    if (!impl->vertex)
    {
        platform::DestroyShaderModule(impl->fragment);
        return nullptr;
    }

    return shader;
}

platform::ShaderModule* GetVertexShader(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->vertex;
}

platform::ShaderModule* GetFragmentShader(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->fragment;
}

int GetCullMode(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->cull;
}

bool IsBlendEnabled(Shader* shader)
{
    return (static_cast<ShaderImpl*>(shader)->flags & shader_flags_blend) != 0;
}

int GetSrcBlend(Shader* shader)
{
    return static_cast<ShaderImpl*>(shader)->src_blend;
}

int GetDstBlend(Shader* shader)
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

const char* GetShaderName(Shader* shader)
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