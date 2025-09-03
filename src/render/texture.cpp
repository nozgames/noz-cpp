//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../internal.h"
#include "../platform.h"

struct TextureImpl : Texture
{
    platform::Texture* platform_texture = nullptr;
    SamplerOptions sampler_options;
    Vec2Int size;
};

static SamplerOptions g_default_sampler_options = {
    TEXTURE_FILTER_LINEAR,
    TEXTURE_CLAMP_CLAMP,
};

static int GetBytesPerPixel(TextureFormat format)
{
    switch (format)
    {
    case TEXTURE_FORMAT_RGBA8:
        return 4;
    case TEXTURE_FORMAT_RGBA16F:
        return 8;
    case TEXTURE_FORMAT_R8:
        return 1;
    default:
        return 4; // Default to RGBA
    }
}


#if 0
SDL_GPUTextureFormat ToSDL(const TextureFormat format)
{
    switch (format)
    {
    case TEXTURE_FORMAT_RGBA8:
        return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    case TEXTURE_FORMAT_RGBA16F:
        return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    case TEXTURE_FORMAT_R8:
        return SDL_GPU_TEXTUREFORMAT_R8_UNORM;
    default:
        return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    }
}
#endif

static void CreateTexture(
    TextureImpl* impl,
    void* data,
    size_t width,
    size_t height,
    TextureFormat format,
    bool generate_mipmaps,
    const Name* name)
{
    assert(impl);
    assert(data);
    assert(width > 0);
    assert(height > 0);

    // Use platform abstraction to create the texture
    impl->platform_texture = platform::CreateTexture(data, width, height, GetBytesPerPixel(format), impl->sampler_options, name->value);
    if (!impl->platform_texture)
        return;
}

Texture* CreateTexture(Allocator* allocator, int width, int height, TextureFormat format, const Name* name)
{
    assert(width > 0);
    assert(height > 0);
    assert(name);

    auto* texture = (Texture*)Alloc(allocator, sizeof(TextureImpl));
    if (!texture)
        return nullptr;

    TextureImpl* impl = static_cast<TextureImpl*>(texture);
    impl->size.x = width;
    impl->size.y = height;
    impl->name = name;
    impl->sampler_options = g_default_sampler_options;
    impl->platform_texture = platform::CreateTexture(nullptr, width, height, GetBytesPerPixel(format), impl->sampler_options, name->value);
    return texture;
}

Texture* CreateTexture(
    Allocator* allocator,
    void* data,
    size_t width,
    size_t height,
    TextureFormat format,
    const Name* name)
{
    assert(data);
    assert(name);

    TextureImpl* impl = (TextureImpl*)Alloc(allocator, sizeof(TextureImpl));
    if (!impl)
        return nullptr;

    CreateTexture(impl, data, width, height, format, false, name);
    impl->sampler_options = g_default_sampler_options;
    return impl;
}

Vec2Int GetSize(Texture* texture)
{
    return static_cast<TextureImpl*>(texture)->size;
}

int GetWidth(Texture* texture)
{
    return static_cast<TextureImpl*>(texture)->size.x;
}

int GetHeight(Texture* texture)
{
    return static_cast<TextureImpl*>(texture)->size.y;
}

SamplerOptions GetSamplerOptions(Texture* texture)
{
    if (!texture)
        return g_default_sampler_options;

    return static_cast<TextureImpl*>(texture)->sampler_options;
}

Asset* LoadTexture(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name)
{
    assert(stream);
    assert(name);
    assert(header);

    TextureFormat format = (TextureFormat)ReadU8(stream);
    uint32_t width = ReadU32(stream);
    uint32_t height = ReadU32(stream);

    TextureImpl* impl = (TextureImpl*)Alloc(allocator, sizeof(TextureImpl));
    if (!impl)
        return nullptr;

    impl->size.x = width;
    impl->size.y = height;

    impl->sampler_options.filter  = (TextureFilter)ReadU8(stream);
    impl->sampler_options.clamp = (TextureClamp)ReadU8(stream);

    const int channels = GetBytesPerPixel(format);
    const size_t data_size = width * height * channels;
    if (const auto texture_data = (u8*)Alloc(ALLOCATOR_SCRATCH, data_size))
    {
        ReadBytes(stream, texture_data, data_size);
        CreateTexture(impl, texture_data, width, height, format, false, GetName(name->value));
        Free(texture_data);
    }

    return impl;
}

void BindTextureInternal(Texture* texture, i32 slot)
{
    return platform::BindTexture(static_cast<TextureImpl*>(texture)->platform_texture, slot);
}