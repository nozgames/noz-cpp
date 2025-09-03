//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct TextureImpl : Texture
{
    //SDL_GPUTexture* handle;
    SamplerOptions sampler_options;
    Vec2Int size;
};

//static SDL_GPUDevice* g_device = nullptr;
static SamplerOptions g_default_sampler_options = {
    TEXTURE_FILTER_LINEAR,
    TEXTURE_FILTER_LINEAR,
    TEXTURE_CLAMP_CLAMP,
    TEXTURE_CLAMP_CLAMP,
    TEXTURE_CLAMP_CLAMP,
//    SDL_GPU_COMPAREOP_INVALID
};

static void texture_destroy_impl(TextureImpl* impl);
int GetBytesPerPixel(TextureFormat format);

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
    int channels,
    bool generate_mipmaps,
    const Name* name)
{
    assert(impl);
    assert(data);
    assert(width > 0);
    assert(height > 0);
    assert(channels > 0);

    // Handle different channel formats
    uint8_t* rgba_data = nullptr;
    bool allocated_rgba = false;

    if (channels == 1)
    {
        // Single channel - use as-is for R8_UNORM
        // No conversion needed
    }
    // Convert RGB to RGBA
    else if (channels == 3)
    {
        const uint8_t* rgb_src = (const uint8_t*)data;
        // todo: use allocator
        rgba_data = (uint8_t*)malloc(width * height * 4);
        if (!rgba_data)
            return;

        allocated_rgba = true;

        for (size_t i = 0; i < width * height; ++i)
        {
            rgba_data[i * 4 + 0] = rgb_src[i * 3 + 0]; // R
            rgba_data[i * 4 + 1] = rgb_src[i * 3 + 1]; // G
            rgba_data[i * 4 + 2] = rgb_src[i * 3 + 2]; // B
            rgba_data[i * 4 + 3] = 255;                // A
        }
        data = rgba_data;
        channels = 4;
    }
    else if (channels != 4)
    {
        return;
    }

    // Create transfer buffer for pixel data
    const size_t pitch = width * channels;
    const size_t size = pitch * height;

#if 0
    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = (Uint32)size;
    transfer_info.props = 0;
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(g_device, &transfer_info);
    if (!transfer_buffer)
    {
        if (allocated_rgba)
            free(rgba_data);
        return;
    }

    // Map transfer buffer and copy pixel data
    void* mapped = SDL_MapGPUTransferBuffer(g_device, transfer_buffer, false);
    if (!mapped)
    {
        SDL_ReleaseGPUTransferBuffer(g_device, transfer_buffer);
        if (allocated_rgba)
            free(rgba_data);
        return;
    }
    SDL_memcpy(mapped, data, size);
    SDL_UnmapGPUTransferBuffer(g_device, transfer_buffer);

    // Calculate number of mipmap levels if requested
    uint32_t num_levels = 1;
    if (generate_mipmaps)
    {
        // Calculate maximum number of mipmap levels
        size_t max_dim = (width > height) ? width : height;
        num_levels = 1 + (u32)floor(log2((double)max_dim));
    }

    // Create GPU texture with appropriate format based on channel count
    SDL_GPUTextureCreateInfo texture_info = {};
    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.format = (channels == 1) ? SDL_GPU_TEXTUREFORMAT_R8_UNORM : SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture_info.width = (int)width;
    texture_info.height = (int)height;
    texture_info.layer_count_or_depth = 1;
    texture_info.num_levels = num_levels;
    texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    texture_info.props = SDL_CreateProperties();
    SDL_SetStringProperty(texture_info.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, name->value);

    impl->handle = SDL_CreateGPUTexture(g_device, &texture_info);
    SDL_DestroyProperties(texture_info.props);
    if (!impl->handle)
    {
        SDL_ReleaseGPUTransferBuffer(g_device, transfer_buffer);
        if (allocated_rgba)
            free(rgba_data);
        return;
    }

    SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(g_device);
    if (!cb)
    {
        SDL_ReleaseGPUTransferBuffer(g_device, transfer_buffer);
        if (allocated_rgba)
            free(rgba_data);
        return;
    }

    // Upload pixel data to GPU texture
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cb);
    SDL_GPUTextureTransferInfo source = {transfer_buffer, 0, (uint32_t)width, (uint32_t)height};
    SDL_GPUTextureRegion destination = {0};
    destination.texture = impl->handle;
    destination.w = (uint32_t)width;
    destination.h = (uint32_t)height;
    destination.d = 1;

    SDL_UploadToGPUTexture(copy_pass, &source, &destination, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cb);
    SDL_ReleaseGPUTransferBuffer(g_device, transfer_buffer);

    impl->size.x = (int)width;
    impl->size.y = (int)height;

    if (allocated_rgba)
        free(rgba_data);
#endif
}

Texture* CreateTexture(Allocator* allocator, int width, int height, TextureFormat format, const Name* name)
{
    assert(width > 0);
    assert(height > 0);
    assert(name);
//    assert(g_device);

    auto* texture = (Texture*)Alloc(allocator, sizeof(TextureImpl));
    if (!texture)
        return nullptr;

    TextureImpl* impl = static_cast<TextureImpl*>(texture);
    impl->size.x = width;
    impl->size.y = height;
    impl->name = name;
    impl->sampler_options = g_default_sampler_options;

#if 0
    SDL_GPUTextureCreateInfo texture_info = {};
    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.format = ToSDL(format);
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture_info.width = width;
    texture_info.height = height;
    texture_info.layer_count_or_depth = 1;
    texture_info.num_levels = 1;
    texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    texture_info.props = SDL_CreateProperties();

    SDL_SetStringProperty(texture_info.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, name->value);
    impl->handle = SDL_CreateGPUTexture(g_device, &texture_info);
    SDL_DestroyProperties(texture_info.props);
#endif

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

    TextureImpl* texture = (TextureImpl*)Alloc(allocator, sizeof(TextureImpl));
    if (!texture)
        return nullptr;

    TextureImpl* impl = static_cast<TextureImpl*>(texture);
    CreateTexture(impl, data, width, height, GetBytesPerPixel(format), false, name);
    impl->sampler_options = g_default_sampler_options;
    return texture;
}

#if 0
static void texture_destroy_impl(TextureImpl* impl)
{
    assert(impl);
    
    if (impl->handle)
		SDL_ReleaseGPUTexture(g_device, impl->handle);
}
#endif

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

/*
SDL_GPUTexture* GetGPUTexture(Texture* texture)
{
    return static_cast<TextureImpl*>(texture)->handle;
}
*/

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

    // Read texture data
    uint32_t format = ReadU32(stream);
    uint32_t width = ReadU32(stream);
    uint32_t height = ReadU32(stream);

    // Validate format
    if (format > 1)
        return nullptr;

    // Create texture object
    TextureImpl* impl = (TextureImpl*)Alloc(allocator, sizeof(TextureImpl));
    if (!impl)
        return nullptr;

//    impl->handle = nullptr;
    impl->size.x = width;
    impl->size.y = height;

    // Read sampler options
    impl->sampler_options.min_filter = (TextureFilter)ReadU8(stream);
    impl->sampler_options.mag_filter = (TextureFilter)ReadU8(stream);
    impl->sampler_options.clamp_u = (TextureClamp)ReadU8(stream);
    impl->sampler_options.clamp_v = (TextureClamp)ReadU8(stream);
    impl->sampler_options.clamp_w = (TextureClamp)ReadU8(stream);
    bool mips = ReadBool(stream);

    if (mips)
    {
        // Read number of mip levels
        uint32_t num_mip_levels = ReadU32(stream);

        // For now, just read the base level and let GPU handle mipmaps
        // TODO: Upload all mip levels to GPU
        for (uint32_t level = 0; level < num_mip_levels; ++level)
        {
            /*uint32_t mip_width =*/ReadU32(stream);
            /*uint32_t mip_height = */ ReadU32(stream);
            uint32_t mip_data_size = ReadU32(stream);

            if (level == 0)
            {
                // todo: use allocator
                u8* mip_data = (u8*)malloc(mip_data_size);
                if (mip_data)
                {
                    ReadBytes(stream, mip_data, mip_data_size);
                    CreateTexture(impl, mip_data, width, height, (format == 1) ? 4 : 3, true, name);
                    free(mip_data);
                }
                else
                {
                    // Skip data if allocation failed
                    SetPosition(stream, GetPosition(stream) + mip_data_size);
                }
            }
            else
            {
                // Skip other mip levels for now
                SetPosition(stream, GetPosition(stream) + mip_data_size);
            }
        }
    }
    else
    {
        const int channels = (format == 1) ? 4 : 3;
        const size_t data_size = width * height * channels;
        if (const auto texture_data = (u8*)Alloc(ALLOCATOR_SCRATCH, data_size))
        {
            ReadBytes(stream, texture_data, data_size);
            CreateTexture(impl, texture_data, width, height, channels, false, GetName(name->value));
            Free(texture_data);
        }
    }

    return impl;
}

int GetBytesPerPixel(TextureFormat format)
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
void InitTexture(RendererTraits* traits, SDL_GPUDevice* device)
{
    g_device = device;
}

void ShutdownTexture()
{
    g_device = nullptr;
}

#endif