//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
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
};

Material* CreateMaterial(Allocator* allocator, Shader* shader)
{
    auto texture_count = 1;
    auto textures_size = texture_count * sizeof(Texture*);
    auto material_size = sizeof(MaterialImpl) + textures_size;

    auto material = (Material*)Alloc(allocator, material_size);
    if (!material)
        return nullptr;

    MaterialImpl* impl = static_cast<MaterialImpl*>(material);
    impl->shader = shader;
    impl->texture_count = texture_count;
    impl->textures = (Texture**)(impl + 1);
    return impl;
}

Shader* GetShader(Material* material)
{
    return static_cast<MaterialImpl*>(material)->shader;
}

void SetTexture(Material* material, Texture* texture, size_t index)
{
    MaterialImpl* impl = static_cast<MaterialImpl*>(material);
    assert(index < impl->texture_count);
    impl->textures[index] = texture;
}

void BindMaterialInternal(Material* material)
{
    MaterialImpl* impl = static_cast<MaterialImpl*>(material);
    
    BindShaderInternal(impl->shader);
    
    for (size_t i = 0, c = impl->texture_count; i < c; ++i)
    {
        int slot = static_cast<int>(SAMPLER_REGISTER_TEX0) + static_cast<int>(i);
        BindTextureInternal(impl->textures[i], slot);
    }
}
