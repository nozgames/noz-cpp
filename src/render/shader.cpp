//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

struct ShaderImpl : Shader
{
    platform::Shader* platform;
    ShaderFlags flags;
};

void ShaderDestructor(void* p)
{
    ShaderImpl* impl = static_cast<ShaderImpl*>(p);
    platform::DestroyShader(impl->platform);
}

Asset* LoadShader(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name)
{
    assert(stream);
    assert(header);
    assert(name);

    ShaderImpl* impl = (ShaderImpl*)Alloc(allocator, sizeof(ShaderImpl), ShaderDestructor);
    if (!impl)
        return nullptr;

    impl->name = name;

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

    impl->flags = (ShaderFlags)ReadU8(stream);
    impl->platform = platform::CreateShader(vertex_bytecode, vertex_bytecode_length, fragment_bytecode, fragment_bytecode_length, impl->flags, name->value);

    Free(vertex_bytecode);
    Free(fragment_bytecode);

    if (!impl->platform)
    {
        Free(impl);
        return nullptr;
    }

    return impl;
}

void BindShaderInternal(Shader* shader)
{
    assert(shader);
    platform::BindShader(static_cast<ShaderImpl*>(shader)->platform);
}
