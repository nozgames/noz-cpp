//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
// 

#define INITIAL_CACHE_SIZE 32

struct Sampler
{
    SDL_GPUSampler* gpu_sampler;
};

static SDL_GPUDevice* g_device = nullptr;
static Sampler* g_cache_samplers = nullptr;
static u64* g_cache_keys = nullptr;
static Map g_cache = {};

static u64 Hash(const SamplerOptions* options)
{
    return Hash((void*)options, sizeof(SamplerOptions));
}

SDL_GPUFilter ToSDL(TextureFilter filter)
{
    switch (filter)
    {
    case TEXTURE_FILTER_NEAREST:
        return SDL_GPU_FILTER_NEAREST;
    default:
    case TEXTURE_FILTER_LINEAR:
        return SDL_GPU_FILTER_LINEAR;
    }
}

SDL_GPUSamplerAddressMode ToSDL(TextureClamp mode)
{
    switch (mode)
    {
    case TEXTURE_CLAMP_REPEAT:
        return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    case TEXTURE_CLAMP_CLAMP:
        return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    case TEXTURE_CLAMP_REPEAT_MIRRORED:
        return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
    default:
        return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    }
}

SDL_GPUSampler* GetGPUSampler(Texture* texture)
{
    assert(g_device);
    assert(texture);

    SamplerOptions options = GetSamplerOptions(texture);
    u64 key = Hash(&options);

    auto* sampler = (Sampler*)GetValue(g_cache, key);
    if (sampler != nullptr)
    {
        assert(sampler->gpu_sampler);
        return sampler->gpu_sampler;
    }

    // Create new sampler
    SDL_GPUSamplerCreateInfo sampler_info = {};
    sampler_info.min_filter = ToSDL(options.min_filter);
    sampler_info.mag_filter = ToSDL(options.mag_filter);
    sampler_info.mipmap_mode = (options.min_filter == TEXTURE_FILTER_NEAREST)
        ? SDL_GPU_SAMPLERMIPMAPMODE_NEAREST
        : SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler_info.address_mode_u = ToSDL(options.clamp_u);
    sampler_info.address_mode_v = ToSDL(options.clamp_v);
    sampler_info.address_mode_w = ToSDL(options.clamp_w);
    sampler_info.enable_compare = sampler_info.compare_op != SDL_GPU_COMPAREOP_INVALID;
    sampler_info.compare_op = options.compare_op;
    sampler_info.props = 0;

    SDL_GPUSampler* gpu_sampler = SDL_CreateGPUSampler(g_device, &sampler_info);
    if (!gpu_sampler)
    {
        ExitOutOfMemory("failed to create sampler");
        return nullptr;
    }

    // Store in cache
    sampler = (Sampler*)SetValue(g_cache, key);
    if (!sampler)
    {
        ExitOutOfMemory("exceeded sampler cache");
        return nullptr;
    }

    sampler->gpu_sampler = gpu_sampler;
    return sampler->gpu_sampler;
}

void InitSamplerFactory(RendererTraits* traits, SDL_GPUDevice* dev)
{
    g_device = dev;
    g_cache_keys = (u64*)Alloc(nullptr, sizeof(u64) * traits->max_samplers);
    g_cache_samplers = (Sampler*)Alloc(nullptr, sizeof(Sampler*) * traits->max_samplers);
    Init(g_cache, g_cache_keys, g_cache_samplers, traits->max_samplers, sizeof(Sampler));
}

void ShutdownSamplerFactory()
{
    assert(g_cache_samplers);
    assert(g_cache_keys);

    Free(g_cache_samplers);
    Free(g_cache_keys);

    g_cache_samplers = nullptr;
    g_cache_keys = nullptr;
    g_cache = {};
    g_device = nullptr;
}
