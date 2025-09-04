//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shellscalingapi.h>

void InitVulkan(const RendererTraits* traits, platform::Window* window);
void ResizeVulkan(const Vec2Int& screen_size);
void ShutdownVulkan();

static Vec2Int g_screen_size;
static Vec2 g_cached_mouse_position = {0, 0};

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
            // Clear with black for now
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
            EndPaint(hwnd, &ps);
        }
        return 0;
    case WM_SIZE:
        {
            // Update global screen size when window is resized
            RECT rect;
            GetClientRect(hwnd, &rect);
            Vec2Int new_size = { rect.right - rect.left, rect.bottom - rect.top };
            if (g_screen_size != new_size)
            {
                g_screen_size = new_size;
                ResizeVulkan(new_size);
            }
        }
        return 0;
    case WM_MOUSEMOVE:
        {
            // Cache mouse position for smooth tracking
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            g_cached_mouse_position.x = static_cast<f32>(x);
            g_cached_mouse_position.y = static_cast<f32>(y);
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

platform::Window* platform::CreatePlatformWindow(const ApplicationTraits* traits)
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
        traits->title,              // Window text
        WS_OVERLAPPEDWINDOW,        // Window style
        CW_USEDEFAULT, CW_USEDEFAULT, traits->width, traits->height,
        NULL,       // Parent window
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional data
    );

    // Initial screen size
    RECT rect;
    GetClientRect(hwnd, &rect);
    g_screen_size = { rect.right - rect.left, rect.bottom - rect.top };

    InitVulkan(&traits->renderer, (Window*)hwnd);

    if (hwnd != NULL)
    {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }

    return (Window*)hwnd;
}

void platform::DestroyPlatformWindow(Window* window)
{
    if (!window)
        return;

    ::DestroyWindow((HWND)window);
}

bool platform::ProcessWindowEvents(Window* window, bool& has_focus, Vec2Int& screen_size)
{
    MSG msg = {};
    bool running = true;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
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
    }

    // Update focus state
    HWND active_window = GetActiveWindow();
    has_focus = (active_window == (HWND)window);

    // Update screen size - get actual physical pixels for DPI awareness
    if (window)
    {
        RECT rect;
        GetClientRect((HWND)window, &rect);
        screen_size.x = rect.right - rect.left;
        screen_size.y = rect.bottom - rect.top;

        if (g_screen_size!= screen_size && screen_size != VEC2INT_ZERO)
        {
            g_screen_size = screen_size;
            ResizeVulkan(screen_size);
        }
    }

    return running;
}

Vec2Int platform::GetWindowSize(Window* window)
{
    if (!window)
        return VEC2INT_ZERO;

    return g_screen_size;
}
    
void platform::ShowCursor(bool show)
{
    ::ShowCursor(show ? TRUE : FALSE);
}

Vec2 platform::GetCachedMousePosition()
{
    return g_cached_mouse_position;
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
