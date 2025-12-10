//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "editor/editor_client.h"
#include "platform.h"
#include <filesystem>
#include <exception>

#include <cstdarg>

static constexpr int FRAME_HISTORY_SIZE = 240;

extern void LoadRendererAssets(Allocator* allocator);
extern void InitRandom();
extern void InitUI(const ApplicationTraits* traits);
extern void InitEvent(ApplicationTraits* traits);
extern void InitName(ApplicationTraits* traits);
extern void InitVfx();
extern void InitTime();
extern void InitJobs();
extern void InitRenderer(const RendererTraits* traits);
extern void InitAllocator(ApplicationTraits* traits);
extern void InitAudio();
extern void InitPrefs(const ApplicationTraits& traits);
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
extern void ShutdownPrefs();
extern void ResetInputState(InputSet* input_set);
extern void UpdateHttp();
extern void UpdateWebSocket();

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
    .max_prefs = 256,
    .max_event_stack = 32,
    .editor_port = 8080,
    .ui_depth = F32_MAX,
    .renderer = {
        .max_frame_commands = 8192 * 2,
        .vsync = 1,
        .msaa = true,
        .min_depth = -10.0f,
        .max_depth = 10.0f,
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
    Vec2Int size = PlatformGetWindowSize();
    if (size == VEC2INT_ZERO || size == g_app.screen_size)
        return;

    g_app.screen_size = size;
    g_app.screen_aspect_ratio = (float)g_app.screen_size.x / (float)g_app.screen_size.y;
}

#ifdef NOZ_EDITOR
static void HandleHotload(EventId event_id, const void* event_data) {
    (void)event_id;
    const AssetLoadedEvent* hotload_event = (const AssetLoadedEvent*)event_data;
    if (g_app.traits.hotload_asset)
        g_app.traits.hotload_asset(hotload_event->name, hotload_event->type);
}
#endif

// @init
void InitApplication(ApplicationTraits* traits) {
    traits = traits ? traits : &g_default_traits;

    g_app = {};
    g_app.title = traits->title;
    g_app.traits = *traits;
    g_app.running = true;

    PlatformInit(&g_app.traits);

    std::filesystem::path binary_path = PlatformGetBinaryPath();
    g_app.binary_path = binary_path.string();
    g_app.binary_dir = binary_path.parent_path().string();
    InitAllocator(traits);
    InitName(traits);
    InitRandom();
    InitPrefs(g_app.traits);
    InitEvent(traits);
    InitTime();
    InitJobs();
    InitTween();
    InitAudio();
    InitHttp();
    InitWebSocket();

    g_app.traits.x = GetPrefInt(PREF_WINDOW_X, g_app.traits.x);
    g_app.traits.y = GetPrefInt(PREF_WINDOW_Y, g_app.traits.y);
    g_app.traits.width = GetPrefInt(PREF_WINDOW_WIDTH, g_app.traits.width);
    g_app.traits.height = GetPrefInt(PREF_WINDOW_HEIGHT, g_app.traits.height);
}

static void HandleClose() {
    g_app.running = false;
}

void InitWindow() {
    assert(!g_app.window_created);

    g_app.window_created = true;

    PlatformInitWindow(HandleClose);

    UpdateScreenSize();

    InitInput();
    InitRenderer(&g_app.traits.renderer);
    InitPhysics();

    if (g_app.traits.load_assets)
        g_app.traits.load_assets(g_app.asset_allocator);

    g_app.asset_allocator = CreateArenaAllocator(g_app.traits.asset_memory_size, "assets");

    LoadRendererAssets(g_app.asset_allocator);

#ifdef NOZ_EDITOR
    if (g_app.traits.editor_port != 0) {
        Listen(EVENT_HOTLOAD, HandleHotload);
        InitEditorClient("127.0.0.1", g_app.traits.editor_port);
    }
#endif // NOZ_EDITOR

    InitVfx();
    InitUI(&g_app.traits);
    InitDebug();
}

void ShutdownWindow() {
    assert(g_app.window_created);

    noz::RectInt window_rect = PlatformGetWindowRect();
    SetPrefInt(PREF_WINDOW_X, window_rect.x);
    SetPrefInt(PREF_WINDOW_Y, window_rect.y);
    SetPrefInt(PREF_WINDOW_WIDTH, window_rect.width);
    SetPrefInt(PREF_WINDOW_HEIGHT, window_rect.height);

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

    ShutdownDebug();
    ShutdownUI();
    ShutdownVfx();
    ShutdownPhysics();
    ShutdownRenderer();

    g_app.window_created = false;
}

// @run - Called each frame by platform main loop
void RunApplicationFrame() {
    if (!g_app.running)
        return;

    if (!UpdateApplication())
        return;

    if (g_app.traits.update)
        g_app.traits.update();
}

// Returns true while app should keep running
bool IsApplicationRunning() {
    return g_app.running;
}

// Request application to exit
void RequestApplicationExit() {
    g_app.running = false;
}

// @shutdown
void ShutdownApplication() {
    if (g_app.traits.shutdown)
        g_app.traits.shutdown();

    if (g_app.window_created)
        ShutdownWindow();

    ShutdownWebSocket();
    ShutdownHttp();
    ShutdownTween();
    ShutdownJobs();
    ShutdownTime();
    ShutdownAudio();
    ShutdownInput();
    PlatformShutdown();
    ShutdownEvent();
    ShutdownName();
    ShutdownPrefs();
    ShutdownAllocator();
}

void FocusApplication() {
    if (!g_app.window_created)
        return;

    PlatformFocusWindow();
}

static void UpdateFPS() {
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
bool UpdateApplication() {
    ClearScratch();

    bool had_focus = PlatformIsWindowFocused();
    PlatformUpdate();

    g_app.has_focus = PlatformIsWindowFocused();

    if (had_focus != g_app.has_focus) {
        ResetInputState(GetInputSet());
        FocusChangedEvent event = { g_app.has_focus };
        Send(EVENT_FOCUS_CHANGED, &event);
    }

    UpdateScreenSize();
    UpdateTime();
    UpdateInput();
    UpdateHttp();
    UpdateWebSocket();

#ifdef NOZ_EDITOR
    UpdateEditorClient();
#endif

    UpdateFPS();

    return g_app.running;
}

bool IsWindowFocused() {
    return g_app.has_focus;
}

// @screen
Vec2Int GetScreenSize()
{
    return g_app.screen_size;
}

Vec2 GetScreenCenter() {
    return {
        static_cast<f32>(g_app.screen_size.x) * 0.5f,
        static_cast<f32>(g_app.screen_size.y) * 0.5f
    };
}

float GetScreenAspectRatio() {
    return g_app.screen_aspect_ratio;
}

void SetSystemCursor(SystemCursor cursor) {
    PlatformSetCursor(cursor);
}

const ApplicationTraits* GetApplicationTraits() {
    return &g_app.traits;
}

float GetCurrentFPS() {
    return static_cast<float>(g_app.average_fps);
}

const char* GetBinaryDirectory() {
    return g_app.binary_dir.c_str();
}

void ThrowError(const char* fmt, ...) {
    assert(fmt);

    va_list args;
    va_start(args, fmt);
    char error_message[4096];
    Format(error_message, sizeof(error_message), fmt, args);
    va_end(args);

    throw std::runtime_error(error_message);
}

void SetPaletteTexture(Texture* texture) {
    extern void SetVfxPaletteTexture(Texture*);
    extern void SetUIPaletteTexture(Texture*);

    SetVfxPaletteTexture(texture);
    SetUIPaletteTexture(texture);
}

bool WriteSaveFile(const char* path, Stream* stream) {
    SaveStream(stream, PlatformGetSaveGamePath() / path);
    return true;
}

Stream* ReadSaveFile(Allocator* allocator, const char* path) {
    return LoadStream(allocator, PlatformGetSaveGamePath() / path);
}
