//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "editor/editor_client.h"
#include "platform.h"
#include "platform/windows/windows_vulkan.h"
#include <cstdio>
#include <filesystem>

#include <cstdarg>

static constexpr int FRAME_HISTORY_SIZE = 240;

extern void LoadRendererAssets(Allocator* allocator);
extern void InitRandom();
extern void InitUI();
extern void InitEvent(ApplicationTraits* traits);
extern void InitName(ApplicationTraits* traits);
extern void InitVfx();
extern void InitTime();
extern void InitJobs();
extern void InitRenderer(const RendererTraits* traits);
extern void InitAllocator(ApplicationTraits* traits);
extern void InitAudio();
extern void UpdateTime();
extern void ShutdownRenderer();
extern void ShutdownEvent();
extern void ShutdownUI();
extern void ShutdownName();
extern void ShutdownJobs();
extern void ShutdownVfx();
extern void ShutdownTime();
extern void ShutdownAllocator();
extern void ShutdownAudio();
extern void ResetInputState(InputSet* input_set);

// @traits
static ApplicationTraits g_default_traits = 
{
    .name = "noz",
    .title = "noz",
    .assets_path = "assets",
    .x = -1,
    .y = -1,
    .width = 800,
    .height = 600,
    .asset_memory_size = 32 * noz::MB,
    .scratch_memory_size = 8 * noz::MB,
    .max_names = 1024,
    .name_memory_size = 1 * noz::MB,
    .max_events = 128,
    .max_event_listeners = 4,
    .max_event_stack = 32,
    .editor_port = 8080,
    .renderer =
    {
        .max_textures = 32,
        .max_shaders = 32,
        .max_samplers = 16,
        .max_pipelines = 64,
        .max_meshes = 256,
        .max_fonts = 8,
        .max_frame_commands = 4096,
        .max_frame_objects = 128,
        .max_frame_transforms = 1024,
        .shadow_map_size = 2048,
        .vsync = 1,
        .msaa = true
    }
};

// @impl
struct Application
{
    bool has_focus;
    bool vsync;
    Vec2Int screen_size;
    float screen_aspect_ratio;
    const char* title;
    ApplicationTraits traits;
    Allocator* asset_allocator;
    double frame_times[FRAME_HISTORY_SIZE];
    int frame_index;
    double accumulated_time;
    double average_fps;
    bool window_created;
    bool running;
    std::string binary_path;
    std::string binary_dir;
};

static Application g_app = {};

void Init(ApplicationTraits& traits)
{
    memcpy(&traits, &g_default_traits, sizeof(ApplicationTraits));
}

// @error
void Exit(const char* format, ...)
{
    extern void LogImpl(LogType, const char*, va_list);

    va_list args;
    va_start(args, format);
    LogImpl(LOG_TYPE_ERROR, format, args);
    va_end(args);
    exit(1);
}

void ExitOutOfMemory(const char* message)
{
    if (message)
        Exit("out_of_memory: %s", message);
    else
        Exit("out_of_memory");
}

static void UpdateScreenSize()
{
    Vec2Int size = platform::GetScreenSize();
    if (size == VEC2INT_ZERO || size == g_app.screen_size)
        return;

    g_app.screen_size = size;
    g_app.screen_aspect_ratio = (float)g_app.screen_size.x / (float)g_app.screen_size.y;
}

#ifdef NOZ_EDITOR
static void HandleHotload(EventId event_id, const void* event_data)
{
    (void)event_id;
    const HotloadEvent* hotload_event = (const HotloadEvent*)event_data;
    if (g_app.traits.hotload_asset)
        g_app.traits.hotload_asset(hotload_event->asset_name);
}
#endif

// @init
void InitApplication(ApplicationTraits* traits, int argc, const char* argv[])
{
    (void)argc;

    traits = traits ? traits : &g_default_traits;

    memset(&g_app, 0, sizeof(Application));
    g_app.title = traits->title;
    g_app.traits = *traits;
    g_app.running = true;
    g_app.binary_path = argv[0];
    g_app.binary_dir = std::filesystem::path(argv[0]).parent_path().string();

    platform::InitApplication(&g_app.traits);

    InitAllocator(traits);
    InitName(traits);
    InitRandom();
    LoadPrefs();
    InitEvent(traits);
    InitTime();
    InitJobs();
    InitAudio();

    g_app.traits.x = GetPrefInt(GetName("window.x"), g_app.traits.x);
    g_app.traits.y = GetPrefInt(GetName("window.y"), g_app.traits.y);
    g_app.traits.width = GetPrefInt(GetName("window.width"), g_app.traits.width);
    g_app.traits.height = GetPrefInt(GetName("window.height"), g_app.traits.height);
}

static void HandleClose()
{
    g_app.running = false;
}

void InitWindow()
{
    assert(!g_app.window_created);

    g_app.window_created = true;

    platform::InitWindow(HandleClose);

    UpdateScreenSize();

    InitInput();
    InitRenderer(&g_app.traits.renderer);
    InitPhysics();

    if (g_app.traits.load_assets)
        g_app.traits.load_assets(g_app.asset_allocator);

    g_app.asset_allocator = CreateArenaAllocator(g_app.traits.asset_memory_size, "assets");

    LoadRendererAssets(g_app.asset_allocator);

#ifdef NOZ_EDITOR
    if (g_app.traits.editor_port != 0)
    {
        Listen(EVENT_HOTLOAD, HandleHotload);
        InitEditorClient("127.0.0.1", g_app.traits.editor_port);
    }
#endif // NOZ_EDITOR

    InitVfx();
    InitUI();
}

void ShutdownWindow()
{
    assert(g_app.window_created);

    RectInt window_rect = platform::GetWindowRect();
    SetPrefInt(GetName("window.x"), window_rect.x);
    SetPrefInt(GetName("window.y"), window_rect.y);
    SetPrefInt(GetName("window.width"), window_rect.width);
    SetPrefInt(GetName("window.height"), window_rect.height);

#ifdef NOZ_EDITOR
    // Shutdown editor client
    if (g_app.traits.editor_port != 0)
    {
        Unlisten(EVENT_HOTLOAD, HandleHotload);
        ShutdownEditorClient();
    }
#endif // NOZ_EDITOR

    if (g_app.traits.unload_assets)
        g_app.traits.unload_assets();

    if (g_app.asset_allocator)
    {
        Destroy(g_app.asset_allocator);
        g_app.asset_allocator = nullptr;
    }

    ShutdownUI();
    ShutdownVfx();
    ShutdownPhysics();
    ShutdownRenderer();

    g_app.window_created = false;
}

// @shutdown
void ShutdownApplication()
{
    if (g_app.window_created)
        ShutdownWindow();

    ShutdownJobs();
    ShutdownTime();
    ShutdownAudio();
    ShutdownInput();
    platform::ShutdownApplication();
    ShutdownEvent();
    ShutdownName();
    SavePrefs();
    ShutdownAllocator();
}

bool IsWindowCreated()
{
    return g_app.window_created;
}

void FocusWindow()
{
    if (!g_app.window_created)
        return;

    platform::FocusWindow();
}

static void UpdateFPS()
{
    // Update FPS tracking
    double frame_time = GetFrameTime();
    if (frame_time > 0.0)
    {
        g_app.accumulated_time -= g_app.frame_times[g_app.frame_index];
        g_app.frame_times[g_app.frame_index] = frame_time;
        g_app.accumulated_time += frame_time;
        g_app.frame_index = (g_app.frame_index + 1) % FRAME_HISTORY_SIZE;

        if (g_app.accumulated_time > 0.0)
            g_app.average_fps = FRAME_HISTORY_SIZE / g_app.accumulated_time;
    }
}

// @update
bool UpdateApplication()
{
    ClearScratch();

    bool had_focus = platform::HasFocus();
    platform::UpdateApplication();
    if (!IsWindowCreated())
        return true;

    bool has_focus = platform::HasFocus();

    if (had_focus != has_focus)
    {
        ResetInputState(GetInputSet());
        FocusChangedEvent event = { has_focus };
        Send(EVENT_FOCUS_CHANGED, &event);
    }

    UpdateScreenSize();
    UpdateTime();
    UpdateInput();

#ifdef NOZ_EDITOR
    UpdateEditorClient();
#endif

    UpdateFPS();

    return g_app.running;
}

// @screen
Vec2Int GetScreenSize()
{
    return g_app.screen_size;
}

Vec2 GetScreenCenter()
{
    return {(f32)g_app.screen_size.x * 0.5f, (f32)g_app.screen_size.y * 0.5f};
}

float GetScreenAspectRatio()
{
    return g_app.screen_aspect_ratio;
}

void ShowCursor(bool cursor)
{
    platform::ShowCursor(cursor);
}

void SetCursor(SystemCursor cursor)
{
    platform::SetCursor(cursor);
}

const ApplicationTraits* GetApplicationTraits()
{
    return &g_app.traits;
}

float GetCurrentFPS()
{
    return (float)g_app.average_fps;
}

const char* GetBinaryDirectory()
{
    return g_app.binary_dir.c_str();
}

void ThrowError(const char* fmt, ...)
{
    assert(fmt);

    va_list args;
    va_start(args, fmt);
    char error_message[4096];
    Format(error_message, sizeof(error_message), fmt, args);
    va_end(args);

    throw std::exception(error_message);
}