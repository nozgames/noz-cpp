//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../internal.h"
#include "../platform.h"

Texture** TEXTURE = nullptr;
Atlas** ATLAS = nullptr;
Texture* ATLAS_ARRAY = nullptr;
int TEXTURE_COUNT = 0;
int ATLAS_COUNT = 0;

struct TextureImpl : Texture {
    PlatformTexture* platform_texture = nullptr;
    SamplerOptions sampler_options;
    TextureFormat format;
    Vec2Int size;
    bool is_array = false;
    int layer_count = 1;
};

struct AtlasImpl : Atlas {
    void* pixel_data = nullptr;
    Vec2Int size;
    TextureFormat format;
    SamplerOptions sampler_options;
};

static SamplerOptions g_default_sampler_options = {
    TEXTURE_FILTER_LINEAR,
    TEXTURE_CLAMP_CLAMP,
};

int GetBytesPerPixel(TextureFormat format) {
    switch (format) {
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

static void CreateTexture(
    TextureImpl* impl,
    void* data,
    size_t width,
    size_t height,
    TextureFormat format,
    const Name* name) {
    assert(impl);
    assert(data);
    assert(width > 0);
    assert(height > 0);

    impl->size = { static_cast<i32>(width), static_cast<i32>(height) };
    impl->format = format;
    impl->platform_texture = PlatformCreateTexture(
        data,
        width,
        height,
        GetBytesPerPixel(format),
        impl->sampler_options,
        name->value);
}

Texture* CreateTexture(Allocator* allocator, int width, int height, TextureFormat format, const Name* name, TextureFilter filter) {
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
    impl->sampler_options.filter = filter;
    impl->platform_texture = PlatformCreateTexture(
        nullptr,
        width,
        height,
        GetBytesPerPixel(format),
        impl->sampler_options,
        name->value);
    return texture;
}

Texture* CreateTexture(
    Allocator* allocator,
    void* data,
    size_t width,
    size_t height,
    TextureFormat format,
    const Name* name,
    TextureFilter filter) {
    assert(data);
    assert(name);

    TextureImpl* impl = static_cast<TextureImpl*>(Alloc(allocator, sizeof(TextureImpl)));
    if (!impl)
        return nullptr;

    impl->sampler_options = g_default_sampler_options;
    impl->sampler_options.filter = filter;
    CreateTexture(impl, data, width, height, format, name);
    return impl;
}

Vec2Int GetSize(Texture* texture) {
    return static_cast<TextureImpl*>(texture)->size;
}

void UpdateTexture(Texture* texture, void* data) {
    if (!texture || !data) return;
    TextureImpl* impl = static_cast<TextureImpl*>(texture);
    PlatformUpdateTexture(impl->platform_texture, data);
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

static void LoadTextureInternal(TextureImpl* impl, Stream* stream, const Name* name) {
    impl->format = (TextureFormat)ReadU8(stream);
    impl->sampler_options.filter  = (TextureFilter)ReadU8(stream);
    impl->sampler_options.clamp = (TextureClamp)ReadU8(stream);
    impl->size.x = ReadU32(stream);
    impl->size.y = ReadU32(stream);

    const int channels = GetBytesPerPixel(impl->format);
    const u32 data_size = impl->size.x * impl->size.y * channels;
    if (const auto texture_data = (u8*)Alloc(ALLOCATOR_SCRATCH, data_size)) {
        ReadBytes(stream, texture_data, data_size);
        CreateTexture(impl, texture_data, impl->size.x, impl->size.y, impl->format, GetName(name->value));
        Free(texture_data);
    }
}

Asset* LoadTexture(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
    (void)name_table;
    (void)header;

    assert(stream);
    assert(name);
    assert(header);

    TextureImpl* impl = (TextureImpl*)Alloc(allocator, sizeof(TextureImpl));
    if (!impl)
        return nullptr;

    LoadTextureInternal(impl, stream, name);

    return impl;
}

Asset* LoadAtlas(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
    (void)name_table;
    (void)header;

    assert(stream);
    assert(name);
    assert(header);

    AtlasImpl* impl = static_cast<AtlasImpl*>(Alloc(allocator, sizeof(AtlasImpl)));
    if (!impl)
        return nullptr;

    impl->name = name;
    impl->format = static_cast<TextureFormat>(ReadU8(stream));
    impl->sampler_options.filter = static_cast<TextureFilter>(ReadU8(stream));
    impl->sampler_options.clamp = static_cast<TextureClamp>(ReadU8(stream));
    impl->size.x = ReadU32(stream);
    impl->size.y = ReadU32(stream);

    const int channels = GetBytesPerPixel(impl->format);
    const u32 data_size = impl->size.x * impl->size.y * channels;
    impl->pixel_data = Alloc(allocator, data_size);
    if (impl->pixel_data)
        ReadBytes(stream, impl->pixel_data, data_size);

    return impl;
}

void BindTextureInternal(Texture* texture, i32 slot) {
    if (!texture)
        texture = TEXTURE_WHITE;

    TextureImpl* impl = static_cast<TextureImpl*>(texture);
    return PlatformBindTexture(impl->platform_texture, slot);
}

#if !defined(NOZ_BUILTIN_ASSETS)

void ReloadTexture(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table) {
    (void)header;
    (void)name_table;

    assert(asset);
    assert(stream);
    TextureImpl* impl = static_cast<TextureImpl*>(asset);

    PlatformFree(impl->platform_texture);
    impl->platform_texture = nullptr;

    LoadTextureInternal(impl, stream, GetName(asset));
}

#endif

// Texture array support

Texture* CreateTextureArray(Allocator* allocator, Atlas** atlases, int atlas_count, const Name* name) {
    if (atlas_count <= 0 || !atlases) return nullptr;

    // Get dimensions from first atlas - all must match
    AtlasImpl* first = static_cast<AtlasImpl*>(atlases[0]);
    int width = first->size.x;
    int height = first->size.y;
    int channels = GetBytesPerPixel(first->format);

    // Collect pixel data pointers from atlases
    void** layer_data = (void**)Alloc(ALLOCATOR_SCRATCH, atlas_count * sizeof(void*));
    for (int i = 0; i < atlas_count; i++) {
        AtlasImpl* atlas = static_cast<AtlasImpl*>(atlases[i]);
        layer_data[i] = atlas->pixel_data;
    }

    TextureImpl* impl = static_cast<TextureImpl*>(Alloc(allocator, sizeof(TextureImpl)));
    if (!impl) {
        Free(layer_data);
        return nullptr;
    }

    impl->name = name;
    impl->size = {width, height};
    impl->format = first->format;
    impl->sampler_options = first->sampler_options;
    impl->is_array = true;
    impl->layer_count = atlas_count;

    impl->platform_texture = PlatformCreateTextureArray(
        layer_data,
        atlas_count,
        width,
        height,
        channels,
        impl->sampler_options,
        name->value);

    Free(layer_data);

    // Free pixel data from atlases - no longer needed
    for (int i = 0; i < atlas_count; i++) {
        AtlasImpl* atlas = static_cast<AtlasImpl*>(atlases[i]);
        if (atlas->pixel_data) {
            Free(atlas->pixel_data);
            atlas->pixel_data = nullptr;
        }
    }

    return impl;
}

bool IsTextureArray(Texture* texture) {
    if (!texture) return false;
    return static_cast<TextureImpl*>(texture)->is_array;
}

int GetLayerCount(Texture* texture) {
    if (!texture) return 0;
    return static_cast<TextureImpl*>(texture)->layer_count;
}
