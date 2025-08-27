//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Allocator;

struct ApplicationTraits
{
    const char* title;
    int width;
    int height;
    size_t asset_memory_size;
    size_t scratch_memory_size;
    size_t max_names;
    size_t name_memory_size;
    size_t max_events;
    size_t max_event_listeners;
    size_t max_event_stack;
    RendererTraits renderer;
    bool (*load_assets)(Allocator* allocator);
    void (*unload_assets)();
    void (*hotload_asset)(const name_t* name);
};

void Init(ApplicationTraits& traits);

void InitApplication(ApplicationTraits* traits);
void ShutdownApplication();
bool UpdateApplication();
void BeginRenderFrame();
void EndRenderFrame();

void Exit(const char* format, ...);
void ExitOutOfMemory(const char* message=nullptr);

ivec2 GetScreenSize();
float GetScreenAspectRatio();

// @time
float GetDeltaTime();
float GetFixedTime();
void GetFixedTimeRate(int rate);
float GetTotalTime();