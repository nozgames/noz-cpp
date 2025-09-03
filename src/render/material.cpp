//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

void BindShaderInternal(Shader* shader);

struct MaterialImpl : Material
{    
    int vertex_uniform_count;
    int fragment_uniform_count;
    Shader* shader;
    Texture** textures;
    size_t texture_count;
    u8* uniforms_data;
};

Material* CreateMaterial(Allocator* allocator, Shader* shader)
{
    auto texture_count = GetSamplerCount(shader);
    auto textures_size = texture_count * sizeof(Texture*);
    auto uniform_data_size = GetUniformDataSize(shader);
    auto material_size =
        sizeof(MaterialImpl) +
        textures_size +
        uniform_data_size;

    auto material = (Material*)Alloc(allocator, material_size);
    if (!material)
        return nullptr;

    MaterialImpl* impl = static_cast<MaterialImpl*>(material);
    impl->shader = shader;
    impl->vertex_uniform_count = GetVertexUniformCount(shader);
    impl->fragment_uniform_count = GetFragmentUniformCount(shader);
    impl->texture_count = texture_count;
    impl->textures = (Texture**)(impl + 1);
    impl->uniforms_data = (u8*)(impl->textures + texture_count);
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
    
    // Bind the shader (pipeline)
    BindShaderInternal(impl->shader);
    
    // TODO: Implement uniform data binding when uniform system is ready
    // PushUniformDataGPU(impl->shader, impl->uniforms_data);

    // TODO: Implement texture binding when texture system is ready
    // for (size_t i = 0, c = impl->texture_count; i < c; ++i)
    //     BindTextureGPU(impl->textures[i], static_cast<int>(i) + static_cast<int>(sampler_register_user0));
}
