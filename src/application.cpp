//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <cstdio>

static constexpr int FRAME_HISTORY_SIZE = 60;

void LoadRendererAssets(Allocator* allocator);
void InitRandom();
void InitUI();
void InitEvent(ApplicationTraits* traits);
void InitName(ApplicationTraits* traits);
void InitVfx();
void ShutdownEvent();
void ShutdownUI();
void ShutdownName();
void ShutdownVfx();

// @traits
static ApplicationTraits g_default_traits = 
{
    .title = "noz",
    .width = 800,
    .height = 600,
    .asset_memory_size = 32 * noz::MB,
    .scratch_memory_size = 8 * noz::MB,
    .max_names = 1024,
    .name_memory_size = 1 * noz::MB,
    .max_events = 128,
    .max_event_listeners = 4,
    .max_event_stack = 32,
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
        .vsync = 0
    }
};

// @impl
struct Application
{
    SDL_Window* window;
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
};

static Application g_app = {};

void Init(ApplicationTraits& traits)
{
    memcpy(&traits, &g_default_traits, sizeof(ApplicationTraits));
}

// @error
void Exit(const char* format, ...)
{
    char buffer[1024];
    
    if (format)
    {
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        fprintf(stderr, "error: %s\n", buffer);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, g_app.title, buffer, NULL);
    }
    else
    {
        fprintf(stderr, "error: unknown error\n");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, g_app.title, "unknown error", NULL);
    }
    
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
    int w;
    int h;
    SDL_GetWindowSize(g_app.window, &w, &h);
    g_app.screen_size = { w, h };
    g_app.screen_aspect_ratio = (float)w / (float)h;
}

#ifdef NOZ_EDITOR
void OnHotload(const char* asset_name)
{
    auto name = GetName(asset_name);

    if (g_app.traits.hotload_asset)
        g_app.traits.hotload_asset(name);
}
#endif

// @init
void InitApplication(ApplicationTraits* traits)
{
    traits = traits ? traits : &g_default_traits;

    InitAllocator(traits);
    InitName(traits);
    InitRandom();
    InitEvent(traits);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD) != 1)
        return;

    Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    memset(&g_app, 0, sizeof(Application));
    g_app.title = traits->title;
    g_app.window = SDL_CreateWindow(traits->title, traits->width, traits->height, windowFlags);
    if (!g_app.window)
    {
        SDL_Quit();
        return;
    }

    g_app.traits = *traits;

    UpdateScreenSize();

    InitInput();
    InitRenderer(&traits->renderer, g_app.window);
    InitTime();
    InitPhysics();

    g_app.asset_allocator = CreateArenaAllocator(traits->asset_memory_size, "assets");

    if (traits->load_assets)
        traits->load_assets(g_app.asset_allocator);

    LoadRendererAssets(g_app.asset_allocator);

#ifdef NOZ_EDITOR
    SetHotloadCallback(OnHotload);
    InitEditorClient("127.0.0.1", 8080);
#endif // NOZ_EDITOR

    InitVfx();
    InitUI();
}

// @shutdown
void ShutdownApplication()
{
#ifdef NOZ_EDITOR
    // Shutdown editor client
    ShutdownEditorClient();
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
    ShutdownTime();
    ShutdownRenderer();
    ShutdownInput();
    ShutdownEvent();
    ShutdownName();
    ShutdownAllocator();
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
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
            return false;

        if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
            g_app.has_focus = true;
        else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
            g_app.has_focus = false;
        else if (event.type == SDL_EVENT_WINDOW_RESIZED)
            UpdateScreenSize();
    }

    UpdateTime();
    UpdateInput();

#ifdef NOZ_EDITOR
    UpdateEditorClient();
#endif

    UpdateFPS();

    return true;
}

// @screen
Vec2Int GetScreenSize()
{
    return g_app.screen_size;
}

float GetScreenAspectRatio()
{
    return g_app.screen_aspect_ratio;
}


void ShowCursor(bool cursor)
{
    if (cursor)
        SDL_ShowCursor();
    else
        SDL_HideCursor();
}

float GetCurrentFPS()
{
    return (float)g_app.average_fps;
}