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

Texture* CreateTexture(Allocator* allocator, int width, int height, TextureFormat format, const Name* name) {
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
    const Name* name) {
    assert(data);
    assert(name);

    TextureImpl* impl = static_cast<TextureImpl*>(Alloc(allocator, sizeof(TextureImpl)));
    if (!impl)
        return nullptr;

    impl->sampler_options = g_default_sampler_options;
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

void BindTextureInternal(Texture* texture, i32 slot) {
    if (!texture)
        texture = TEXTURE_WHITE;

    return PlatformBindTexture(static_cast<TextureImpl*>(texture)->platform_texture, slot);
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

Texture* CreateTextureArray(Allocator* allocator, Texture** textures, int texture_count, const Name* name) {
    if (texture_count <= 0 || !textures) return nullptr;

    // Get dimensions from first texture - all must match
    TextureImpl* first = static_cast<TextureImpl*>(textures[0]);
    int width = first->size.x;
    int height = first->size.y;
    int channels = GetBytesPerPixel(first->format);

    // Collect pixel data pointers (would need to load pixels from GPU or keep CPU copies)
    // For now, this requires textures that still have their data available
    // This is a limitation - in practice, atlas textures would need to provide their pixel data

    // Allocate array of data pointers
    void** layer_data = (void**)Alloc(ALLOCATOR_SCRATCH, texture_count * sizeof(void*));
    for (int i = 0; i < texture_count; i++) {
        // Note: This assumes textures have CPU-accessible pixel data
        // In practice, you'd need to read back from GPU or maintain CPU copies
        layer_data[i] = nullptr;  // Placeholder - actual implementation needs pixel access
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
    impl->layer_count = texture_count;

    impl->platform_texture = PlatformCreateTextureArray(
        layer_data,
        texture_count,
        width,
        height,
        channels,
        impl->sampler_options,
        name->value);

    Free(layer_data);
    return impl;
}

void BindTextureArray(Texture* texture_array, int slot) {
    if (!texture_array) return;
    TextureImpl* impl = static_cast<TextureImpl*>(texture_array);
    if (!impl->is_array) {
        // Fall back to regular binding if not an array
        PlatformBindTexture(impl->platform_texture, slot);
        return;
    }
    PlatformBindTextureArray(impl->platform_texture, slot);
}

bool IsTextureArray(Texture* texture) {
    if (!texture) return false;
    return static_cast<TextureImpl*>(texture)->is_array;
}

int GetLayerCount(Texture* texture) {
    if (!texture) return 0;
    return static_cast<TextureImpl*>(texture)->layer_count;
}
