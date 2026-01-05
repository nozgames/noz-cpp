//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../internal.h"
#include "../platform.h"

extern void BindShaderInternal(Shader* shader);
extern void BindTextureInternal(Texture* texture, i32 slot);

struct MaterialImpl : Material
{    
    Shader* shader;
    Texture** textures;
    size_t texture_count;
    u8 vertex_data[MAX_UNIFORM_BUFFER_SIZE];
    u8 fragment_data[MAX_UNIFORM_BUFFER_SIZE];
    bool has_vertex_data;
    bool has_fragment_data;
};

Material* CreateMaterial(Allocator* allocator, Shader* shader)
{
    constexpr u32 texture_count = 2;  // Support main texture + lightmap
    u32 textures_size = texture_count * (u32)sizeof(Texture*);
    u32 material_size = (u32)sizeof(MaterialImpl) + textures_size;

    auto material = (Material*)Alloc(allocator, material_size);
    if (!material)
        return nullptr;

    MaterialImpl* impl = static_cast<MaterialImpl*>(material);
    impl->shader = shader;
    impl->texture_count = texture_count;
    impl->textures = reinterpret_cast<Texture**>(impl + 1);
    impl->name = shader->name;
    impl->has_vertex_data = false;
    impl->has_fragment_data = false;

    // Initialize all texture slots to nullptr
    for (size_t i = 0; i < texture_count; i++) {
        impl->textures[i] = nullptr;
    }

    return impl;
}

Shader* GetShader(Material* material)
{
    return static_cast<MaterialImpl*>(material)->shader;
}

Texture* GetTexture(Material* material, size_t index)
{
    assert(material);
    assert(index < static_cast<MaterialImpl*>(material)->texture_count);
    return static_cast<MaterialImpl*>(material)->textures[index];
}

void SetTexture(Material* material, Texture* texture, size_t index) {
    MaterialImpl* impl = static_cast<MaterialImpl*>(material);
    assert(index < impl->texture_count);
    impl->textures[index] = texture;
}

void BindMaterialInternal(Material* material)
{
    MaterialImpl* impl = static_cast<MaterialImpl*>(material);
    
    BindShaderInternal(impl->shader);
    
    for (size_t i = 0, c = impl->texture_count; i < c; ++i)
        if (impl->textures[i])
            BindTextureInternal(impl->textures[i], static_cast<int>(i));

    if (impl->has_vertex_data)
        PlatformBindVertexUserData(impl->vertex_data, MAX_UNIFORM_BUFFER_SIZE);

    if (impl->has_fragment_data)
        PlatformBindFragmentUserData(impl->fragment_data, MAX_UNIFORM_BUFFER_SIZE);
}

void SetVertexData(Material* material, const void* data, size_t size) {
    MaterialImpl* impl = static_cast<MaterialImpl*>(material);
    assert(size <= MAX_UNIFORM_BUFFER_SIZE);
    memcpy(impl->vertex_data, data, size);
    impl->has_vertex_data = true;
}

void SetFragmentData(Material* material, const void* data, size_t size) {
    MaterialImpl* impl = static_cast<MaterialImpl*>(material);
    assert(size <= MAX_UNIFORM_BUFFER_SIZE);
    memcpy(impl->fragment_data,  data, size);
    impl->has_fragment_data = true;
}
