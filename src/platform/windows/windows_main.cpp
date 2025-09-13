    //
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"

constexpr int INPUT_BUFFER_SIZE = 1024;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <shlobj.h>
#include <filesystem>
#include <thread>

extern void InitVulkan(const RendererTraits* traits, HWND hwnd);
extern void ResizeVulkan(const Vec2Int& screen_size);
extern void ShutdownVulkan();
extern void WaitVulkan();
extern void HandleInputCharacter(char c);
extern void HandleInputKeyDown(char c);

struct WindowsApp
{
    const ApplicationTraits* traits;
    Vec2Int screen_size;
    Vec2 mouse_position;
    Vec2 mouse_scroll;
    HWND hwnd;
    bool has_focus;
    bool is_resizing;
    void (*on_close) ();
    char input_buffer[INPUT_BUFFER_SIZE];
    int input_buffer_start;
    HCURSOR cursor_default;
    HCURSOR cursor_wait;
    HCURSOR cursor_cross;
    HCURSOR cursor_move;
    SystemCursor cursor;
};

static WindowsApp g_windows = {};

// Method to get focus state for input system
bool GetWindowFocus()
{
    return g_windows.has_focus;
}

void ThreadSleep(int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void ThreadYield()
{
    std::this_thread::yield();
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        if (g_windows.on_close)
            g_windows.on_close();
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_SIZE:
    {
        // Update global screen size when window is resized
        RECT rect;
        GetClientRect(hwnd, &rect);
        Vec2Int new_size = { rect.right - rect.left, rect.bottom - rect.top };
        if (g_windows.screen_size != new_size)
        {
            g_windows.screen_size = new_size;
            ResizeVulkan(new_size);
        }
    }
        return 0;

    case WM_ENTERSIZEMOVE:
        g_windows.is_resizing = true;
        break;

    case WM_EXITSIZEMOVE:
        g_windows.is_resizing = false;
        break;

    case WM_MOUSEWHEEL:
    {
        // Get wheel delta (positive = forward/up, negative = backward/down)
        int wheel_delta = GET_WHEEL_DELTA_WPARAM(wParam);
        // Normalize to a reasonable scroll value (Windows default is 120 per notch)
        g_windows.mouse_scroll.y += static_cast<f32>(wheel_delta) / 120.0f;
    }
        break;
    case WM_MOUSEMOVE:
    {
        // Cache mouse position for smooth tracking
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        g_windows.mouse_position = {
            static_cast<f32>(x),
            static_cast<f32>(y)
        };
        return 0;
    }

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT)
        {
            SetCursor(g_windows.cursor);
            return 0;
        }
        break;

    case WM_CHAR:
        HandleInputCharacter((char)wParam);
        return 0;

    case WM_KEYDOWN:
        HandleInputKeyDown((char)wParam);
        return 0;

    case WM_SYSCHAR:
        return 0;

    case WM_SYSKEYDOWN:
        return 0;

    case WM_KEYUP:
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void platform::InitApplication(const ApplicationTraits* traits)
{
    g_windows = {};
    g_windows.traits = traits;
    g_windows.cursor = SYSTEM_CURSOR_DEFAULT;
    g_windows.cursor_default = LoadCursor(nullptr, IDC_ARROW);
    g_windows.cursor_wait = LoadCursor(nullptr, IDC_WAIT);
    g_windows.cursor_cross = LoadCursor(nullptr, IDC_CROSS);
    g_windows.cursor_move = LoadCursor(nullptr, IDC_SIZEALL);
}

void platform::InitWindow(void (*on_close)())
{
    // Set process to be DPI aware (per-monitor DPI awareness)
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Register window class
    const char* CLASS_NAME = "NoZGameWindow";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    RegisterClass(&wc);

    // Create window
    HWND hwnd = ::CreateWindowEx(
        0,                          // Optional window styles
        CLASS_NAME,                 // Window class
        g_windows.traits->title,              // Window text
        WS_OVERLAPPEDWINDOW,        // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,
        g_windows.traits->width,
        g_windows.traits->height,
        NULL,       // Parent window
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional data
    );

    g_windows.hwnd = hwnd;
    g_windows.on_close = on_close;

    // Initial screen size
    RECT rect;
    GetClientRect(hwnd, &rect);
    g_windows.screen_size = { rect.right - rect.left, rect.bottom - rect.top };

    InitVulkan(&g_windows.traits->renderer, hwnd);

    if (hwnd != NULL)
    {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
}

void platform::ShutdownApplication()
{
    if (g_windows.hwnd)
        ::DestroyWindow((HWND)g_windows.hwnd);
}

bool platform::HasFocus()
{
    return g_windows.has_focus;
}

bool platform::IsResizing()
{
    return g_windows.is_resizing;
}

bool platform::UpdateApplication()
{
    MSG msg = {};
    bool running = true;

    g_windows.mouse_scroll = {0, 0};

    int count = 0;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && count < 10)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        count++;
    }

    g_windows.has_focus = GetActiveWindow() == g_windows.hwnd;

    RECT rect;
    GetClientRect(g_windows.hwnd, &rect);
    Vec2Int screen_size = {
        rect.right - rect.left,
        rect.bottom - rect.top
    };

    if (g_windows.screen_size != screen_size && screen_size != VEC2INT_ZERO)
    {
        g_windows.screen_size = screen_size;
        ResizeVulkan(screen_size);
    }

    return running;
}

Vec2Int platform::GetScreenSize()
{
    return g_windows.screen_size;
}

void platform::SetCursor(SystemCursor cursor)
{
    g_windows.cursor = cursor;

    switch (cursor)
    {
        default:
        case SYSTEM_CURSOR_DEFAULT:
            SetCursor(g_windows.cursor_default);
            break;

        case SYSTEM_CURSOR_MOVE:
            SetCursor(g_windows.cursor_move);
            break;

        case SYSTEM_CURSOR_SELECT:
            SetCursor(g_windows.cursor_cross);
            break;

        case SYSTEM_CURSOR_WAIT:
            SetCursor(g_windows.cursor_wait);
            break;
    }
}

void platform::ShowCursor(bool show)
{
    ::ShowCursor(show ? TRUE : FALSE);
}

// Vec2 platform::GetMousePosition()
// {
//     platform::Window* window = ::GetWindow();
//     if (!window)
//         return Vec2{0, 0};
//
//     HWND hwnd = (HWND)window;
//
//     POINT cursor_pos;
//     if (!GetCursorPos(&cursor_pos))
//         return VEC2_ZERO;
//
//     if (!ScreenToClient(hwnd, &cursor_pos))
//         return VEC2_ZERO;
//
//     return Vec2{static_cast<f32>(cursor_pos.x), static_cast<f32>(cursor_pos.y)};
// }


Vec2 platform::GetMousePosition()
{
    return g_windows.mouse_position;
}

Vec2 platform::GetMouseScroll()
{
    return g_windows.mouse_scroll;
}

void platform::FocusWindow()
{
    if (g_windows.hwnd)
        SetForegroundWindow(g_windows.hwnd);
}

std::filesystem::path platform::GetSaveGamePath()
{
    PWSTR appdata_path;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appdata_path)))
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
    
extern int main(int argc, char* argv[]);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Convert command line arguments
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return 1;

    char **args = new char*[argc];
    for (int i = 0; i < argc; i++)
    {
        // Get required buffer size
        int size = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
        if (size > 0)
        {
            char* buffer = new char[size];
            WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, buffer, size, nullptr, nullptr);
            args[i] = buffer;  // or _strdup(buffer) if you need separate ownership
        }
        else
        {
            args[i] = _strdup("");  // Handle conversion failure
        }
    }

    int result = main(argc, args);

    for (int i = 0; i < argc; i++)
        free(args[i]);

    delete[] args;

    return result;
}

void platform::Log(LogType type, const char* message)
{
    (void)type;

    OutputDebugStringA(message);
    OutputDebugStringA("\n");
}