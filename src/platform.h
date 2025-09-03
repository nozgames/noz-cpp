//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef void* WindowHandle;

namespace platform
{
    Vec2 GetMousePosition();
    
    // @window
    WindowHandle CreatePlatformWindow(const ApplicationTraits* traits);
    void DestroyPlatformWindow(WindowHandle window);
    bool ProcessWindowEvents(WindowHandle window, bool& has_focus, Vec2Int& screen_size);
    Vec2Int GetWindowSize(WindowHandle window);
    void ShowCursor(bool show);
    
    // @render
    void SetRendererWindow(WindowHandle window);

    // @time
    u64 GetPerformanceCounter();
    u64 GetPerformanceFrequency();
}

// Global renderer functions
void SetVulkanWindow(WindowHandle window);
void HandleVulkanWindowResize();
