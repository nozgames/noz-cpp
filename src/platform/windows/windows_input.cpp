//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#include <windows.h>

platform::Window* GetWindow();

namespace platform
{
    Vec2 GetMousePosition()
    {
        platform::Window* window = ::GetWindow();
        if (!window)
            return Vec2{0, 0};
            
        HWND hwnd = (HWND)window;
        
        POINT cursor_pos;
        if (GetCursorPos(&cursor_pos))
        {
            // Convert screen coordinates to client coordinates
            if (ScreenToClient(hwnd, &cursor_pos))
            {
                return Vec2{static_cast<f32>(cursor_pos.x), static_cast<f32>(cursor_pos.y)};
            }
        }
        
        return Vec2{0, 0};
    }
}
