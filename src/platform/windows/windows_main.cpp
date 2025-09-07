    //
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <shlobj.h>
#include <filesystem>

extern void InitVulkan(const RendererTraits* traits, HWND hwnd);
extern void ResizeVulkan(const Vec2Int& screen_size);
extern void ShutdownVulkan();
extern void WaitVulkan();

struct WindowsApp
{
    const ApplicationTraits* traits;
    Vec2Int screen_size;
    Vec2 mouse_position;
    Vec2 mouse_scroll;
    HWND hwnd;
    bool has_focus;
    bool is_resizing;
};

static WindowsApp g_windows = {};

// Method to get focus state for input system
bool GetWindowFocus()
{
    return g_windows.has_focus;
}

void thread_sleep_ms(int milliseconds)
{
    Sleep(milliseconds);
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
        }
        return 0;
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

                if(g_windows.traits->test)
                {
                    g_windows.traits->test();
                }
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
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void platform::InitApplication(const ApplicationTraits* traits)
{
    g_windows = {};
    g_windows.traits = traits;
}

void platform::InitWindow()
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
        if (msg.message == WM_QUIT)
        {
            running = false;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

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
    // Convert command line arguments
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return 1;

    char* args[1] = { (char*)".exe" };
    return main(1, args);
}
