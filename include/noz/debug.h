//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

extern void InitDebug();
extern void ShutdownDebug();
extern void DrawDebug();
extern void DebugLine(const Vec2& start, const Vec2& end, const Color& color, float width=0.02f);
extern void DebugBounds(const Bounds2& bounds, const Mat3& transform, const Color& color, float width=0.02f);
