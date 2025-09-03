//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define INITIAL_CACHE_SIZE 64
#include "../platform.h"

struct Pipeline
{
    platform::Pipeline* platform_pipeline;
    platform::PipelineLayout* platform_layout;
};

static Map g_cache = {};
static u64* g_cache_keys = nullptr;
static Pipeline* g_cache_pipelines = nullptr;

static uint64_t MakeKey(Shader* shader)
{
    struct
    {
        void* shader_ptr;
    } key_data = {shader};

    return Hash(&key_data, sizeof(key_data));
}

platform::Pipeline* GetPipeline(Shader* shader)
{
    assert(shader);

    auto key = MakeKey(shader);
    auto* pipeline = (Pipeline*)GetValue(g_cache, key);
    if (pipeline != nullptr)
        return pipeline->platform_pipeline;

    // Create new platform pipeline
    platform::Pipeline* platform_pipeline = platform::CreatePipeline(shader);
    if (!platform_pipeline)
        return nullptr;

    pipeline = (Pipeline*)SetValue(g_cache, key, nullptr);
    if (!pipeline)
    {
        // Clean up the pipeline if we can't cache it
        platform::DestroyPipeline(platform_pipeline);
        ExitOutOfMemory("pipeline limit exceeded");
    }

    pipeline->platform_pipeline = platform_pipeline;
    pipeline->platform_layout = nullptr; // TODO: Store actual layout when implemented
    return pipeline->platform_pipeline;
}

void InitPipelineFactory(const RendererTraits* traits)
{
    g_cache_keys = (u64*)Alloc(nullptr, sizeof(u64) * traits->max_pipelines);
    g_cache_pipelines = (Pipeline*)Alloc(nullptr, sizeof(Pipeline) * traits->max_pipelines);
    Init(g_cache, g_cache_keys, g_cache_pipelines, traits->max_pipelines, sizeof(Pipeline));
}

void ShutdownPipelineFactory()
{
    Free(g_cache_keys);
    Free(g_cache_pipelines);
    g_cache = {};
}
