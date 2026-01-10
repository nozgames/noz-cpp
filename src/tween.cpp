//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#if 0

constexpr u32 MAX_TWEENS = 128;

struct TweenImpl {
    bool active;
    float elapsed;
    float duration;
    float duration_inv;
    u32 generation;
    Vec4 from;
    Vec4 to;
    TweenFunc func;
    void* user_data;
};

struct TweenSystem
{
    PoolAllocator* allocator;
    u32 next_generation;
};

static TweenSystem g_tween = {};

inline TweenImpl& GetTween(u32 index) { return *(TweenImpl*)GetAt(g_tween.allocator, index); }
inline TweenId MakeTweenId(u32 index, u32 generation) { return {(static_cast<u64>(index) << 32) | static_cast<u64>(generation)}; }

void UpdateTweens()
{
    u32 tween_count = GetCount(g_tween.allocator);
    f32 frame_time = GetFrameTime();
    for (u32 i=0; i<MAX_TWEENS && tween_count > 0; i++)
    {
        Tween& TweenImpl = GetTween(i);
        if (!TweenImpl.active)
            continue;

        tween.elapsed += frame_time;
        if (tween.elapsed >= tween.duration)
        {
            tween.func(tween.to, 1.0f, tween.user_data);
            Free(&tween);
            continue;
        }

        float t = tween.elapsed * tween.duration_inv;
        tween.func(Mix(tween.from, tween.to, t), t, tween.user_data);

        tween_count--;
    }
}

TweenId CreateTween(TweenFunc func, float from, float to, float duration, void* user_data)
{
    return CreateTween(func, Vec4{from, 0, 0, 0}, Vec4{to, 0, 0, 0}, duration, user_data);
}

TweenId CreateTween(TweenFunc func, const Vec2& from, const Vec2& to, float duration, void* user_data)
{
    return CreateTween(func, Vec4{from.x, from.y, 0, 0}, Vec4{to.x, to.y, 0, 0}, duration, user_data);
}

TweenId CreateTween(TweenFunc func, const Vec3& from, const Vec3& to, float duration, void* user_data)
{
    return CreateTween(func, Vec4{from.x, from.y, from.z, 0}, Vec4{to.x, to.y, to.z, 0}, duration, user_data);
}

TweenId CreateTween(TweenFunc func, const Vec4& from, const Vec4& to, float duration, void* user_data)
{
    if (IsFull(g_tween.allocator))
        return INVALID_TWEEN_ID;

    Tween* tween = (Tween*)Alloc(g_tween.allocator, sizeof(Tween));
    assert(tween);

    tween->active = true;
    tween->generation = g_tween.next_generation++;
    tween->duration = duration;
    tween->duration_inv = 1.0f / duration;
    tween->elapsed = 0.0f;
    tween->user_data = user_data;
    tween->func = func;
    tween->from = from;
    tween->to = to;

    tween->func(from, 0.0f, user_data);

    return MakeTweenId(GetIndex(g_tween.allocator, tween), tween->generation);
}

void InitTween()
{
    g_tween = {};
    g_tween.allocator = CreatePoolAllocator(sizeof(Tween), MAX_TWEENS);
    g_tween.next_generation = 1;
}

void ShutdownTween()
{
}

#endif

namespace noz {
    void InitTween() {
    }

    void ShutdownTween() {
    }

    float Tween(float from, float to, float time, float duration) {
        float t = Clamp(time / duration, 0.0f, 1.0f);
        return from + (to - from) * t;
    }

    float Tween(float from, float to, float time, float duration, const std::function<float(float value)>& func) {
        float t = func(Clamp(time / duration, 0.0f, 1.0f));
        float v = from + (to - from) * t;
        return v;
    }
}
