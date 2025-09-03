//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/noz.h>
#include "../../platform.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

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
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

namespace platform
{
    WindowHandle CreatePlatformWindow(const ApplicationTraits* traits)
    {
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
        
        if (hwnd != NULL)
        {
            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);
        }
        
        return (WindowHandle)hwnd;
    }
    
    void DestroyPlatformWindow(WindowHandle window)
    {
        if (window)
        {
            ::DestroyWindow((HWND)window);
        }
    }
    
    bool ProcessWindowEvents(WindowHandle window, bool& has_focus, Vec2Int& screen_size)
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
        
        // Update screen size
        if (window)
        {
            RECT rect;
            GetClientRect((HWND)window, &rect);
            screen_size.x = rect.right - rect.left;
            screen_size.y = rect.bottom - rect.top;
        }
        
        return running;
    }
    
    Vec2Int GetWindowSize(WindowHandle window)
    {
        if (window)
        {
            RECT rect;
            GetClientRect((HWND)window, &rect);
            return { rect.right - rect.left, rect.bottom - rect.top };
        }
        return { 0, 0 };
    }
    
    void ShowCursor(bool show)
    {
        ::ShowCursor(show ? TRUE : FALSE);
    }
    
    void SetRendererWindow(WindowHandle window)
    {
        // Forward to Vulkan renderer function (declared in platform.h)
        SetVulkanWindow(window);
    }
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
