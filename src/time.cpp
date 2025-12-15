//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "platform.h"

const int DEFAULT_FIXED_RATE = 50;

struct Time
{
    int fixed_rate;
    float delta;
    float fixed;
    float total;
    u64 start_time;
    u64 last_frame_time;
    u64 frame_index;
};

static Time g_time = {};

void UpdateTime() {
    g_time.frame_index ++;

    u64 current = PlatformGetTimeCounter();
    u64 frequency = PlatformGetTimeFrequency();

    // Calculate delta time
    u64 delta_ticks = current - g_time.last_frame_time;
    g_time.last_frame_time = current;
    g_time.delta = (float)delta_ticks / (float)frequency;

    // Calculate total time
    u64 total_ticks = current - g_time.start_time;
    g_time.total = (float)total_ticks / (float)frequency;
}

float GetFrameTime() {
    return g_time.delta;
}

float GetFixedTime() {
    return g_time.fixed;
}

float GetTotalTime()
{
    return g_time.total;
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
