//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz {

    // @ui
    extern void ToggleDebugUI(void (*game_section)()=nullptr);
    extern void DebugProperty(const char* name, const char* value);
    extern void DebugProperty(const char* name, int* value);

    // @gizmos
    extern void DrawDebugGizmos();
    extern void DebugLine(const Vec2& start, const Vec2& end, const Color& color, float width=0.02f);
    extern void DebugBounds(const Bounds2& bounds, const Mat3& transform, const Color& color, float width=0.02f);
}
