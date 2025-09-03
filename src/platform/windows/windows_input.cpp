//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/noz.h>
#include <windows.h>

namespace platform
{
    Vec2 GetMousePosition()
    {
        POINT cursor_pos;
        if (GetCursorPos(&cursor_pos))
            return Vec2{ static_cast<f32>(cursor_pos.x), static_cast<f32>(cursor_pos.y) };

        return VEC2_ZERO;
    }
}
