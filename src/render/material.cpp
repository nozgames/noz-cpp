//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct MaterialImpl
{    
    OBJECT_BASE;
    name_t name;
    int vertex_uniform_count;
    int fragment_uniform_count;
    Shader* shader;
    Texture** textures;
    size_t texture_count;
    u8* uniforms_data;
};

static MaterialImpl* Impl(Material* m) { return (MaterialImpl*)Cast(m, TYPE_MATERIAL); }

Material* CreateMaterial(Allocator* allocator, Shader* shader)
{
    auto texture_count = GetSamplerCount(shader);
    auto textures_size = texture_count * sizeof(Texture*);
    auto uniform_data_size = GetUniformDataSize(shader);
    auto material_size =
        sizeof(MaterialImpl) +
        textures_size +
        uniform_data_size;

    auto material = (Material*)CreateObject(allocator, material_size, TYPE_MATERIAL);
    if (!material)
        return nullptr;

    auto impl = Impl(material);
    impl->shader = shader;
    impl->vertex_uniform_count = GetVertexUniformCount(shader);
    impl->fragment_uniform_count = GetFragmentUniformCount(shader);
    impl->texture_count = texture_count;
    impl->textures = (Texture**)(impl + 1);
    impl->uniforms_data = (u8*)(impl->textures + texture_count);
    return (Material*)impl;
}

Shader* GetShader(Material* material)
{
    return Impl(material)->shader;
}

void SetTexture(Material* material, Texture* texture, size_t index)
{
    auto impl = Impl(material);
    assert(index < impl->texture_count);
    impl->textures[index] = texture;
}

void BindMaterialGPU(Material* material, SDL_GPUCommandBuffer* cb)
{
    auto impl = Impl(material);
    BindShaderGPU(impl->shader);
    PushUniformDataGPU(impl->shader, cb, impl->uniforms_data);

    for (size_t i = 0, c = impl->texture_count; i < c; ++i)
        BindTextureGPU(impl->textures[i], cb, static_cast<int>(i) + static_cast<int>(sampler_register_user0));
}
