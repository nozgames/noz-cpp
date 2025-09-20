//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct TweenId
{
    u64 value;
};

constexpr TweenId INVALID_TWEEN_ID = {0};

typedef void (*TweenFunc) (const Vec4& value, float t, void* user_data);

// @tween
TweenId CreateTween(TweenFunc func, float from, float to, float duration, void* user_data);
TweenId CreateTween(TweenFunc func, const Vec2& from, const Vec2& to, float duration, void* user_data);
TweenId CreateTween(TweenFunc func, const Vec3& from, const Vec3& to, float duration, void* user_data);
TweenId CreateTween(TweenFunc func, const Vec4& from, const Vec4& to, float duration, void* user_data);
