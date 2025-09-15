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
    u32 asset_memory_size;
    u32 scratch_memory_size;
    u32 max_names;
    u32 name_memory_size;
    u32 max_events;
    u32 max_event_listeners;
    u32 max_event_stack;
    u16 editor_port;
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
void SetThreadName(const char* name);

// @helper
extern void ThrowError(const char* format, ...);

// @job
struct JobHandle
{
    u32 id;
    u32 version;

    bool operator == (const JobHandle& o) const { return id == o.id && version == o.version; }
    bool operator != (const JobHandle& o) const { return !(*this == o); }
};

constexpr JobHandle INVALID_JOB_HANDLE = { 0, 0 };

typedef void (*JobRunFunc)(void* user_data);

extern JobHandle CreateJob(JobRunFunc func, void* user_data = nullptr, JobHandle depends_on = INVALID_JOB_HANDLE);
extern bool IsDone(JobHandle handle);


