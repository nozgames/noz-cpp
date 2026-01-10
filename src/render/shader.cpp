//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../platform.h"

using namespace noz;

Shader** SHADER = nullptr;
int SHADER_COUNT = 0;

namespace noz {

    struct ShaderImpl : Shader {
        PlatformShader* platform;
        ShaderFlags flags;
    };

    void ShaderDestructor(void* p) {
        ShaderImpl* impl = static_cast<ShaderImpl*>(p);
        PlatformFree(impl->platform);
    }

    static bool LoadShaderInternal(ShaderImpl* impl, Stream* stream, const AssetHeader& header, const Name** name_table) {
        (void)name_table;
        (void)header;

        auto vertex_length = ReadU32(stream);
        char* vertex = static_cast<char *>(Alloc(ALLOCATOR_DEFAULT, vertex_length));
        if (!vertex)
            return false;

        ReadBytes(stream, vertex, vertex_length);

        u32 fragment_length = ReadU32(stream);
        char* fragment = static_cast<char*>(Alloc(ALLOCATOR_DEFAULT, fragment_length));
        if (!fragment) {
            Free(vertex);
            return false;
        }
        ReadBytes(stream, fragment, fragment_length);

        impl->flags = static_cast<ShaderFlags>(ReadU8(stream));

        // Auto-detect shader types by name
        if (impl->name) {
            if (strstr(impl->name->value, "postprocess_ui_composite") != nullptr) {
                // UI composite must be checked first (more specific)
                impl->flags |= SHADER_FLAGS_UI_COMPOSITE;
            } else if (strstr(impl->name->value, "postprocess") != nullptr) {
                impl->flags |= SHADER_FLAGS_POSTPROCESS;
            }
        }

        impl->platform = PlatformCreateShader(
            vertex, vertex_length,
            fragment, fragment_length,
            impl->flags, impl->name->value);

        Free(vertex);
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

    void BindShaderInternal(Shader* shader) {
        if (!shader) return;
        PlatformBindShader(static_cast<ShaderImpl*>(shader)->platform);
    }

#if !defined(NOZ_BUILTIN_ASSETS)

    void ReloadShader(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table) {
        ShaderImpl* impl = static_cast<ShaderImpl*>(asset);
        assert(impl);
        assert(stream);
        assert(header.type == ASSET_TYPE_SHADER);

        PlatformShader* old_shader = impl->platform;
        LoadShaderInternal(impl, stream, header, name_table);

        if (old_shader)
            PlatformFree(old_shader);
    }

#endif
}
