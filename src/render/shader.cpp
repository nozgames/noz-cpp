//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

Shader** SHADER = nullptr;
int SHADER_COUNT = 0;

struct ShaderImpl : Shader {
    platform::Shader* platform;
    ShaderFlags flags;
};

void ShaderDestructor(void* p) {
    ShaderImpl* impl = static_cast<ShaderImpl*>(p);
    platform::DestroyShader(impl->platform);
}

static bool LoadShaderInternal(ShaderImpl* impl, Stream* stream, const AssetHeader& header, const Name** name_table) {
    (void)name_table;
    (void)header;

    auto vertex_length = ReadU32(stream);
    auto* vertex = (u8*)Alloc(ALLOCATOR_DEFAULT, vertex_length);
    if (!vertex)
        return false;

    ReadBytes(stream, vertex, vertex_length);

    u32 geometry_length = ReadU32(stream);
    u8* geometry = nullptr;
    if (geometry_length > 0) {
        geometry = (u8*)Alloc(ALLOCATOR_DEFAULT, geometry_length);
        if (!geometry)
        {
            Free(vertex);
            return false;
        }
        ReadBytes(stream, geometry, geometry_length);
    }

    u32 fragment_length = ReadU32(stream);
    u8* fragment = (u8*)Alloc(ALLOCATOR_DEFAULT, fragment_length);
    if (!fragment) {
        Free(vertex);
        if (geometry) Free(geometry);
        return false;
    }
    ReadBytes(stream, fragment, fragment_length);

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
        vertex, vertex_length,
        geometry, geometry_length,
        fragment, fragment_length,
        impl->flags, impl->name->value);

    Free(vertex);
    if (geometry) Free(geometry);
    Free(fragment);

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
