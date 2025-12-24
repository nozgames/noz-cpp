    //
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#include <resource.h>

constexpr int INPUT_BUFFER_SIZE = 1024;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <shlobj.h>
#include <filesystem>
#include <thread>
#include <dwmapi.h>

extern void InitRenderDriver(const RendererTraits* traits, HWND hwnd);
extern void ResizeRenderDriver(const Vec2Int& screen_size);
extern void ShutdownRenderDriver();
static void SetCursorInternal(SystemCursor cursor);
static void ShowCursorInternal(bool show);
extern void MarkTextboxChanged();
extern HBRUSH GetNativeEditBrush();
extern COLORREF GetNativeEditTextColor();
extern COLORREF GetNativeEditBgColor();

struct WindowsApp {
    const ApplicationTraits* traits;
    noz::RectInt window_rect;
    Vec2Int screen_size;
    Vec2 mouse_position;
    Vec2 mouse_scroll;
    HWND hwnd;
    bool has_focus;
    bool is_resizing;
    bool driver_initialized;
    void (*on_close) ();
    char input_buffer[INPUT_BUFFER_SIZE];
    int input_buffer_start;
    HCURSOR cursor_default;
    HCURSOR cursor_wait;
    HCURSOR cursor_cross;
    HCURSOR cursor_move;
    SystemCursor cursor;
    bool mouse_on_screen;
    bool show_cursor;
};

static WindowsApp g_windows = {};

HWND GetWindowHandle() {
    return g_windows.hwnd;
}

Vec2Int PlatformGetWindowSize() {
    return g_windows.screen_size;
}

bool PlatformIsWindowFocused() {
    return g_windows.has_focus;
}

static BOOL EnableDarkMode(HWND hwnd)
{
    BOOL value = TRUE;

    HRESULT hr = DwmSetWindowAttribute(
        hwnd,
        DWMWA_USE_IMMERSIVE_DARK_MODE,
        &value,
        sizeof(value));

    return SUCCEEDED(hr);
}

void ThreadSleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void ThreadYield() {
    std::this_thread::yield();
}

void PlatformSetThreadName(const char* name) {
    int size = MultiByteToWideChar(CP_UTF8, 0, name, -1, nullptr, 0);
    std::wstring wname(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wname.data(), size);
    SetThreadDescription(GetCurrentThread(), wname.c_str());
}

u64 PlatformGetThreadId() {
    return static_cast<u64>(GetCurrentThreadId());
}

static void UpdateWindowRect() {
    RECT rect;
    GetWindowRect(g_windows.hwnd, &rect);
    g_windows.window_rect = noz::RectInt{
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top
    };

    GetClientRect(g_windows.hwnd, &rect);
    Vec2Int screen_size = {
        rect.right - rect.left,
        rect.bottom - rect.top
    };

    if (screen_size != g_windows.screen_size && screen_size != VEC2INT_ZERO) {
        g_windows.screen_size = screen_size;
        if (g_windows.driver_initialized)
            ResizeRenderDriver(screen_size);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_DESTROY:
            if (g_windows.on_close)
                g_windows.on_close();
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_SIZE:
            UpdateWindowRect();
            return 0;

        case WM_MOVE:
            UpdateWindowRect();
            break;

        case WM_ENTERSIZEMOVE:
            g_windows.is_resizing = true;
            break;

        case WM_EXITSIZEMOVE:
            g_windows.is_resizing = false;
            break;

        case WM_SIZING:
            UpdateWindowRect();
            extern void RunApplicationFrame();
            RunApplicationFrame();
            break;

        case WM_MOUSEWHEEL: {
            int wheel_delta = GET_WHEEL_DELTA_WPARAM(wParam);
            g_windows.mouse_scroll.y += static_cast<f32>(wheel_delta) / 120.0f;
            break;
        }

        case WM_MOUSEMOVE: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            g_windows.mouse_position = {
                static_cast<f32>(x),
                static_cast<f32>(y)
            };

            if (!g_windows.mouse_on_screen) {
                g_windows.mouse_on_screen = true;
                TRACKMOUSEEVENT tme = {};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
                ShowCursorInternal(g_windows.cursor != SYSTEM_CURSOR_NONE);
            }

            return 0;
        }

        case WM_MOUSELEAVE:
            g_windows.mouse_on_screen = false;
            ShowCursorInternal(true);
            break;

        case WM_ACTIVATEAPP:
            if (wParam == FALSE)
                PlatformHideTextbox();
            break;

        case WM_LBUTTONDBLCLK:
            extern void PlatformSetDoubleClick();
            PlatformSetDoubleClick();
            break;


        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                SetCursorInternal(g_windows.cursor);
                return 0;
            }

            g_windows.mouse_on_screen = false;
            ShowCursorInternal(true);
            break;

        case WM_SYSCHAR:
            return 0;

        case WM_SYSKEYDOWN:
            return 0;

        case WM_KEYDOWN:
            return 0;

        case WM_KEYUP:
            return 0;

        case WM_CHAR:
            return 0;

        case WM_COMMAND:
            if (HIWORD(wParam) == EN_CHANGE)
                MarkTextboxChanged();
            break;

        case WM_CTLCOLOREDIT: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, GetNativeEditTextColor());
            SetBkColor(hdc, GetNativeEditBgColor());
            HBRUSH brush = GetNativeEditBrush();
            return reinterpret_cast<LRESULT>(brush);
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void PlatformInit(const ApplicationTraits* traits) {
    g_windows = {};
    g_windows.traits = traits;
    g_windows.cursor = SYSTEM_CURSOR_DEFAULT;
    g_windows.cursor_default = LoadCursor(nullptr, IDC_ARROW);
    g_windows.cursor_wait = LoadCursor(nullptr, IDC_WAIT);
    g_windows.cursor_cross = LoadCursor(nullptr, IDC_CROSS);
    g_windows.cursor_move = LoadCursor(nullptr, IDC_SIZEALL);
}

void PlatformInitWindow(void (*on_close)()) {
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    const char* CLASS_NAME = "NoZGameWindow";
    WNDCLASS wc = {};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClass(&wc);

    HWND hwnd = ::CreateWindowEx(
        0,
        CLASS_NAME,
        g_windows.traits->title,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        g_windows.traits->x < 0 ? CW_USEDEFAULT : g_windows.traits->x,
        g_windows.traits->y < 0 ? CW_USEDEFAULT : g_windows.traits->y,
        g_windows.traits->width,
        g_windows.traits->height,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    HICON hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APP));
    SendMessage(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));

    EnableDarkMode(hwnd);

    g_windows.hwnd = hwnd;
    g_windows.on_close = on_close;
    g_windows.show_cursor = true;
    g_windows.cursor = SYSTEM_CURSOR_DEFAULT;

    UpdateWindowRect();

    InitRenderDriver(&g_windows.traits->renderer, hwnd);
    g_windows.driver_initialized = true;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}

void PlatformShutdown() {
    if (!g_windows.hwnd)
        return;

    ::DestroyWindow(g_windows.hwnd);
    g_windows.hwnd = nullptr;
}


bool PlatformUpdate() {
    MSG msg = {};

    g_windows.mouse_scroll = {0, 0};

    // Clear double-click flag from previous frame before processing new messages
    extern void PlatformClearDoubleClick();
    PlatformClearDoubleClick();

    if (!g_windows.is_resizing) {
        int count = 0;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) && count < 20) {
            //if (!IsDialogMessage(g_windows.hwnd, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            //}
            count++;
        }
    }

    g_windows.has_focus = GetActiveWindow() == g_windows.hwnd;

    return true;
}

static void ShowCursorInternal(bool show) {
    if (g_windows.show_cursor == show)
        return;

    g_windows.show_cursor = show;

    ShowCursor(show ? TRUE : FALSE);
}

static void SetCursorInternal(SystemCursor cursor) {
    if (cursor == SYSTEM_CURSOR_NONE) {
        if (g_windows.mouse_on_screen) {
            ShowCursorInternal(false);
        }
        return;
    }

    if (cursor == SYSTEM_CURSOR_DEFAULT) {
        SetCursor(g_windows.cursor_default);
    } else if (cursor == SYSTEM_CURSOR_MOVE) {
        SetCursor(g_windows.cursor_move);
    } else if (cursor == SYSTEM_CURSOR_SELECT) {
        SetCursor(g_windows.cursor_cross);
    } else if (cursor == SYSTEM_CURSOR_WAIT) {
        SetCursor(g_windows.cursor_wait);
    }

    ShowCursorInternal(true);
}

void PlatformSetCursor(SystemCursor cursor) {
    g_windows.cursor = cursor;
    SetCursorInternal(cursor);
}

Vec2 PlatformGetMousePosition() {
    return g_windows.mouse_position;
}

Vec2 PlatformGetMouseScroll() {
    return g_windows.mouse_scroll;
}

void PlatformFocusWindow() {
    if (g_windows.hwnd)
        SetForegroundWindow(g_windows.hwnd);
}

std::filesystem::path PlatformGetSaveGamePath() {
    PWSTR appdata_path;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appdata_path)))
    {
        std::filesystem::path save_path = appdata_path;
        CoTaskMemFree(appdata_path);

        save_path /= g_windows.traits->name;

        std::error_code ec;
        std::filesystem::create_directories(save_path, ec);

        return save_path;
    }

    return std::filesystem::path(".");
}

bool PlatformSavePersistentData(const char* name, const void* data, u32 size) {
    std::filesystem::path path = PlatformGetSaveGamePath() / name;

    FILE* file = fopen(path.string().c_str(), "wb");
    if (!file)
        return false;

    u32 written = static_cast<u32>(fwrite(data, 1, size, file));
    fclose(file);

    return written == size;
}

u8* PlatformLoadPersistentData(Allocator* allocator, const char* name, u32* out_size) {
    std::filesystem::path path = PlatformGetSaveGamePath() / name;

    FILE* file = fopen(path.string().c_str(), "rb");
    if (!file) {
        *out_size = 0;
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    u32 file_size = static_cast<u32>(ftell(file));
    fseek(file, 0, SEEK_SET);

    if (file_size == 0) {
        fclose(file);
        *out_size = 0;
        return nullptr;
    }

    u8* data = static_cast<u8*>(Alloc(allocator, file_size));
    *out_size = static_cast<u32>(fread(data, 1, file_size, file));
    fclose(file);

    return data;
}

std::filesystem::path PlatformGetBinaryPath() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    return std::filesystem::path(path);
}

std::filesystem::path PatformGetCurrentPath() {
    char path[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, path);
    return std::filesystem::path(path);
}

// Parse command-line arguments in key=value format as query params
static void ParseCommandLineQueryParams(int argc, char** argv) {
    InitQueryParams();

    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];
        char* eq = strchr(arg, '=');
        if (eq && eq != arg) {
            // Found '=' - split into name and value
            size_t name_len = eq - arg;
            char name[256];
            if (name_len >= sizeof(name)) name_len = sizeof(name) - 1;
            memcpy(name, arg, name_len);
            name[name_len] = '\0';

            const char* value = eq + 1;
            SetQueryParam(name, value);
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Parse command line arguments
    int argc;
    LPWSTR* argv_wide = CommandLineToArgvW(GetCommandLineW(), &argc);

    // Convert wide strings to UTF-8
    char** argv = (char**)malloc(argc * sizeof(char*));
    for (int i = 0; i < argc; i++) {
        int size = WideCharToMultiByte(CP_UTF8, 0, argv_wide[i], -1, nullptr, 0, nullptr, nullptr);
        argv[i] = (char*)malloc(size);
        WideCharToMultiByte(CP_UTF8, 0, argv_wide[i], -1, argv[i], size, nullptr, nullptr);
    }
    LocalFree(argv_wide);

    InitCommandLine(argc, argv);
    ParseCommandLineQueryParams(argc, argv);

    Main();

    while (IsApplicationRunning())
        RunApplicationFrame();

    ShutdownApplication();

    // Free command line memory
    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);

    return 0;
}

void PlatformLog(LogType type, const char* message)
{
    (void)type;

    std::string temp = message;
    temp += "\n";
    OutputDebugStringA(temp.c_str());
}

noz::RectInt PlatformGetWindowRect() {
    return g_windows.window_rect;
}


bool PlatformIsMouseOverWindow(){
    return g_windows.mouse_on_screen;
}

bool PlatformIsMobile() {
    return false;  // Windows is never mobile
}

bool PlatformIsPortrait() {
    return g_windows.screen_size.y > g_windows.screen_size.x;
}

void PlatformRequestLandscape() {
    // No-op on Windows
}

void PlatformRequestFullscreen() {
    // No-op on Windows - use alt+enter or window controls
}

bool PlatformIsFullscreen() {
    return false;  // TODO: Implement if needed
}

void PlatformOpenUrl(const char* url) {
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
}
