//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"
#include <cstring>

Shader** SHADER = nullptr;

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

static bool LoadShaderInternal(ShaderImpl* impl, Stream* stream, const AssetHeader& header, const Name** name_table)
{
    (void)header;
    (void)name_table;

    // Read the vertex shader
    auto vertex_bytecode_length = ReadU32(stream);
    auto* vertex_bytecode = (u8*)Alloc(ALLOCATOR_DEFAULT, vertex_bytecode_length);
    if (!vertex_bytecode)
        return false;

    ReadBytes(stream, vertex_bytecode, vertex_bytecode_length);

    // Read geometry shader (optional)
    auto geometry_bytecode_length = ReadU32(stream);
    u8* geometry_bytecode = nullptr;
    if (geometry_bytecode_length > 0)
    {
        geometry_bytecode = (u8*)Alloc(ALLOCATOR_DEFAULT, geometry_bytecode_length);
        if (!geometry_bytecode)
        {
            Free(vertex_bytecode);
            return false;
        }
        ReadBytes(stream, geometry_bytecode, geometry_bytecode_length);
    }

    // Read the fragment shader
    auto fragment_bytecode_length = ReadU32(stream);
    auto* fragment_bytecode = (u8*)Alloc(ALLOCATOR_DEFAULT, fragment_bytecode_length);
    if (!fragment_bytecode)
    {
        Free(vertex_bytecode);
        if (geometry_bytecode) Free(geometry_bytecode);
        return false;
    }
    ReadBytes(stream, fragment_bytecode, fragment_bytecode_length);

    impl->flags = (ShaderFlags)ReadU8(stream);

    // Auto-detect shader types by name
    if (impl->name) {
        if (strstr(impl->name->value, "postprocess_ui_composite") != nullptr) {
            // UI composite must be checked first (more specific)
            impl->flags |= SHADER_FLAGS_UI_COMPOSITE;
        } else if (strstr(impl->name->value, "postprocess") != nullptr) {
            impl->flags |= SHADER_FLAGS_POSTPROCESS;
        }
    }

    impl->platform = platform::CreateShader(
        vertex_bytecode, vertex_bytecode_length,
        geometry_bytecode, geometry_bytecode_length,
        fragment_bytecode, fragment_bytecode_length,
        impl->flags, impl->name->value);

    Free(vertex_bytecode);
    if (geometry_bytecode) Free(geometry_bytecode);
    Free(fragment_bytecode);

    return impl->platform != nullptr;
}

Asset* LoadShader(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table)
{
    assert(stream);
    assert(header);
    assert(name);

    ShaderImpl* impl = (ShaderImpl*)Alloc(allocator, sizeof(ShaderImpl), ShaderDestructor);
    if (!impl)
        return nullptr;

    impl->name = name;
    if (!LoadShaderInternal(impl, stream, *header, name_table))
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

#ifdef NOZ_EDITOR

void ReloadShader(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table) {
    ShaderImpl* impl = static_cast<ShaderImpl*>(asset);
    assert(impl);
    assert(stream);
    assert(header.type == ASSET_TYPE_SHADER);

    platform::Shader* old_shader = impl->platform;
    LoadShaderInternal(impl, stream, header, name_table);

    if (old_shader)
        platform::DestroyShader(old_shader);
}

#endif
