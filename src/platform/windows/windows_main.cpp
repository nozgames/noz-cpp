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

extern void InitVulkan(const RendererTraits* traits, HWND hwnd);
extern void ResizeVulkan(const Vec2Int& screen_size);
extern void ShutdownVulkan();
extern void WaitVulkan();
extern void HandleInputCharacter(char c);
extern void HandleInputKeyDown(char c);
static void SetCursorInternal(SystemCursor cursor);
static void ShowCursorInternal(bool show);

struct WindowsApp {
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
    noz::RectInt window_rect;
    SystemCursor cursor;
    bool mouse_on_screen;
    bool show_cursor;
};

static WindowsApp g_windows = {};

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

void platform::SetThreadName(const char* name)
{
    // Convert char* to wide string
    int size = MultiByteToWideChar(CP_UTF8, 0, name, -1, nullptr, 0);
    std::wstring wname(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wname.data(), size);

    SetThreadDescription(GetCurrentThread(), wname.c_str());
}

static void UpdateWindowRect()
{
    RECT rect;
    GetWindowRect(g_windows.hwnd, &rect);
    noz::RectInt window_rect = noz::RectInt{
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top
    };

    if (window_rect.width <= 0 || window_rect.height <= 0)
        return;

    g_windows.window_rect = window_rect;
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
        if (g_windows.screen_size != new_size && new_size != VEC2INT_ZERO)
        {
            g_windows.screen_size = new_size;
            ResizeVulkan(new_size);
        }
        UpdateWindowRect();
        return 0;
    }

    case WM_MOVE:
        UpdateWindowRect();
        break;

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

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            SetCursorInternal(g_windows.cursor);
            return 0;
        }

        g_windows.mouse_on_screen = false;
        ShowCursorInternal(true);
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
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    const char* CLASS_NAME = "NoZGameWindow";
    WNDCLASS wc = {};
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
        WS_OVERLAPPEDWINDOW,
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
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    EnableDarkMode(hwnd);

    g_windows.hwnd = hwnd;
    g_windows.on_close = on_close;
    g_windows.show_cursor = true;
    g_windows.cursor = SYSTEM_CURSOR_DEFAULT;

    // Initial screen size
    RECT rect;
    GetClientRect(hwnd, &rect);
    g_windows.screen_size = { rect.right - rect.left, rect.bottom - rect.top };

    InitVulkan(&g_windows.traits->renderer, hwnd);

    if (hwnd != nullptr)
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

bool platform::HasFocus() {
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
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) && count < 10)
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

void platform::SetCursor(SystemCursor cursor) {
    g_windows.cursor = cursor;
    SetCursorInternal(cursor);
}

Vec2 platform::GetMousePosition() {
    return g_windows.mouse_position;
}

Vec2 platform::GetMouseScroll() {
    return g_windows.mouse_scroll;
}

void platform::FocusWindow() {
    if (g_windows.hwnd)
        SetForegroundWindow(g_windows.hwnd);
}

std::filesystem::path platform::GetSaveGamePath() {
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

    std::string temp = message;
    temp += "\n";
    OutputDebugStringA(temp.c_str());
}

noz::RectInt platform::GetWindowRect() {
    return g_windows.window_rect;
}


bool platform::IsMouseOverWindow(){
    return g_windows.mouse_on_screen;
}

