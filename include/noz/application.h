//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Allocator;

namespace platform
{
    struct Window;
}

struct ApplicationTraits
{
    const char* name;
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
    void (*hotload_asset)(const Name* name);
};

void Init(ApplicationTraits& traits);

void InitApplication(ApplicationTraits* traits);
void ShutdownApplication();
bool UpdateApplication();
void BeginRenderFrame(Color clear_color);
void EndRenderFrame();

void Exit(const char* format, ...);
void ExitOutOfMemory(const char* message=nullptr);

Vec2Int GetScreenSize();
Vec2 GetScreenCenter();
float GetScreenAspectRatio();

void ShowCursor(bool show);

platform::Window* GetWindow();
const ApplicationTraits* GetApplicationTraits();

// @time
float GetFrameTime();
float GetFixedTime();
void GetFixedTimeRate(int rate);
float GetTotalTime();
float GetCurrentFPS();