//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

struct Allocator;

namespace platform
{
    struct Window;
}

enum SystemCursor {
    SYSTEM_CURSOR_NONE,
    SYSTEM_CURSOR_DEFAULT,
    SYSTEM_CURSOR_MOVE,
    SYSTEM_CURSOR_SELECT,
    SYSTEM_CURSOR_WAIT,
};

enum Orientation {
    ORIENTATION_ANY,        // No preference, use native orientation
    ORIENTATION_LANDSCAPE,  // Prefer landscape, rotate if portrait
    ORIENTATION_PORTRAIT,   // Prefer portrait, rotate if landscape
};

struct ApplicationTraits {
    const char* name;
    const char* title;
    const char** asset_paths;       // Null-terminated list of asset search paths (resolved to full paths)
    int x;
    int y;
    int width;
    int height;
    Orientation orientation;        // Preferred screen orientation
    u32 asset_memory_size;
    u32 scratch_memory_size;
    u32 max_names;
    u32 name_memory_size;
    u32 max_events;
    u32 max_event_listeners;
    u32 max_prefs;
    u32 max_event_stack;
    u32 max_tasks;
    u32 max_frame_tasks;
    u32 max_task_worker_count;
    struct {
        u32 max_requests;
        u32 max_concurrent_requests;
    } http;
    float ui_depth;
    RendererTraits renderer;
    bool (*load_assets)(Allocator* allocator);
    void (*unload_assets)();
    void (*hotload_asset)(const Name* incoming_name, AssetType incoming_type);
    void (*draw_cursor)(const Mat3& transform);
    void (*update)();       // Called each frame - game logic goes here
    void (*shutdown)();     // Called on application exit
};

extern void Init(ApplicationTraits& traits);

extern void InitApplication(ApplicationTraits* traits);
extern void InitWindow();
extern void RunApplicationFrame();      // Called each frame by platform
extern bool IsApplicationRunning();     // Check if app should keep running
extern void RequestApplicationExit();   // Request application to exit
extern void ShutdownApplication();
extern void ShutdownWindow();
extern bool IsWindowCreated();
extern void FocusApplication();
extern bool PlatformIsWindowFocused();
extern bool UpdateApplication();
extern void BeginRender(Color clear_color);
extern void EndRender();
extern void SetSystemCursor(SystemCursor cursor);
extern void SetPaletteTexture(Texture* texture);

const char* GetBinaryDirectory();
const char* GetCurrentDirectory();
const char* GetProjectDirectory();

void Exit(const char* format, ...);
void ExitOutOfMemory(const char* message=nullptr);

Vec2Int GetScreenSize();
Vec2 GetScreenCenter();
float GetScreenAspectRatio();
float GetSystemDPIScale(); // Returns system DPI scale factor (1.0 = 96 DPI on Windows)
bool IsScreenRotated();  // True if screen is being rotated to match preferred orientation
bool IsMobile();         // True if running on a mobile device
bool IsFullscreen();     // True if currently in fullscreen mode
void RequestFullscreen(); // Request fullscreen mode (must be called from user gesture on web)
void OpenUrl(const char* url); // Open URL in browser (new tab on web, default browser on Windows)

const ApplicationTraits* GetApplicationTraits();

// @time
extern f32 GetFrameTime();
extern f32 GetFixedTime();
extern void SetFixedTimeRate(int rate);
extern f64 GetTime();
extern f64 GetRealTime();
extern float GetCurrentFPS();
extern u64 GetFrameIndex();
inline u64 GetMilliseconds(f64 time) { return static_cast<u64>(time * 1000.0); }

// @thread
void ThreadYield();
void ThreadSleep(int milliseconds);
void SetThreadName(const char* name);
u64 GetThreadId();
bool IsMainThread();

// @helper
extern void ThrowError(const char* format, ...);

// @save
extern bool WriteSaveFile(const char* path, Stream* stream);
extern Stream* ReadSaveFile(Allocator* allocator, const char* path);

// @cmdline
extern void InitCommandLine(int argc, char** argv);
extern int GetArgCount();
extern const char* GetArg(int index);
extern const char* GetArgValue(const char* name);  // Returns value for --name <value> or nullptr
extern bool HasArg(const char* name);              // Returns true if --name exists

// @query - URL query parameters (web) or command-line key=value pairs (desktop)
extern void InitQueryParams();                      // Called by platform init
extern void SetQueryParam(const char* name, const char* value);  // Set a query param (for platform use)
extern const char* GetQueryParam(const char* name); // Returns value for name=value or nullptr
extern bool HasQueryParam(const char* name);        // Returns true if param exists

