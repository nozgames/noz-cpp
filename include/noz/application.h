//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Allocator;

namespace platform
{
    struct Window;
}

enum SystemCursor
{
    SYSTEM_CURSOR_DEFAULT,
    SYSTEM_CURSOR_MOVE,
    SYSTEM_CURSOR_SELECT,
    SYSTEM_CURSOR_WAIT,
};

struct ApplicationTraits
{
    const char* name;
    const char* title;
    const char* assets_path;
    int width;
    int height;
    size_t asset_memory_size;
    size_t scratch_memory_size;
    size_t max_names;
    size_t name_memory_size;
    size_t max_events;
    size_t max_event_listeners;
    size_t max_event_stack;
    u16 editor_port;
    bool console;
    RendererTraits renderer;
    bool (*load_assets)(Allocator* allocator);
    void (*unload_assets)();
    void (*hotload_asset)(const Name* name);
};

extern void Init(ApplicationTraits& traits);

extern void InitApplication(ApplicationTraits* traits, int argc, const char* argv[]);
extern void InitWindow();
extern void ShutdownApplication();
extern void ShutdownWindow();
extern bool IsWindowCreated();
extern void FocusWindow();
extern bool UpdateApplication();
extern void BeginRenderFrame(Color clear_color);
extern void EndRenderFrame();
extern void ShowCursor(bool show);
extern void SetCursor(SystemCursor cursor);

const char* GetBinaryDirectory();
void Exit(const char* format, ...);
void ExitOutOfMemory(const char* message=nullptr);

Vec2Int GetScreenSize();
Vec2 GetScreenCenter();
float GetScreenAspectRatio();

platform::Window* GetWindow();
const ApplicationTraits* GetApplicationTraits();

// @time
float GetFrameTime();
float GetFixedTime();
void GetFixedTimeRate(int rate);
float GetTotalTime();
float GetCurrentFPS();

// @thread
void ThreadYield();
void ThreadSleep(int milliseconds);

// @helper
extern void ThrowError(const char* format, ...);
