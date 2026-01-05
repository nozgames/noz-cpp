//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "platform.h"

const int DEFAULT_FIXED_RATE = 50;

struct Time
{
    int fixed_rate;
    f32 delta;
    f32 fixed;
    f64 total;
    u64 start_time;
    u64 last_frame_time;
    u64 frame_index;
};

static Time g_time = {};

void UpdateTime() {
    g_time.frame_index ++;

    u64 current = PlatformGetTimeCounter();
    u64 frequency = PlatformGetTimeFrequency();

    u64 delta_ticks = current - g_time.last_frame_time;
    g_time.last_frame_time = current;
    g_time.delta = static_cast<float>(delta_ticks) / static_cast<float>(frequency);

    u64 total_ticks = current - g_time.start_time;
    g_time.total = static_cast<f64>(total_ticks) / static_cast<f64>(frequency);
}

f32 GetFrameTime() {
    return g_time.delta;
}

f32 GetFixedTime() {
    return g_time.fixed;
}

f64 GetTime() {
    return g_time.total;
}

f64 GetRealTime() {
    u64 current = PlatformGetTimeCounter();
    u64 frequency = PlatformGetTimeFrequency();
    u64 total_ticks = current - g_time.start_time;
    return static_cast<f64>(total_ticks) / static_cast<f64>(frequency);
}

u64 GetFrameIndex() {
    return g_time.frame_index;
}


void SetFixedTimeRate(int rate) {
    assert(rate > 0);
    g_time.fixed_rate = rate;
    g_time.fixed = 1.0f / static_cast<float>(g_time.fixed_rate);
}

void InitTime()
{
     g_time.start_time = g_time.last_frame_time = PlatformGetTimeCounter();
     SetFixedTimeRate(DEFAULT_FIXED_RATE);
}

void ShutdownTime()
{
}
