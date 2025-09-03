//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef void* WindowHandle;

namespace platform
{
    Vec2 GetMousePosition();
    
    // Window management
    WindowHandle CreatePlatformWindow(const ApplicationTraits* traits);
    void DestroyPlatformWindow(WindowHandle window);
    bool ProcessWindowEvents(WindowHandle window, bool& has_focus, Vec2Int& screen_size);
    Vec2Int GetWindowSize(WindowHandle window);
    void ShowCursor(bool show);
}
