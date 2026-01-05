//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../../platform.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

u64 PlatformGetTimeCounter() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (u64)counter.QuadPart;
}

u64 PlatformGetTimeFrequency() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return (u64)frequency.QuadPart;
}
