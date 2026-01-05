//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//
//  Web/Emscripten time functions
//

#include "../../platform.h"

#include <emscripten.h>

u64 PlatformGetTimeCounter() {
    // emscripten_get_now() returns milliseconds as double
    // We convert to microseconds for better precision
    double now_ms = emscripten_get_now();
    return (u64)(now_ms * 1000.0);
}

u64 PlatformGetTimeFrequency() {
    // We're returning time in microseconds from PlatformGetTimeCounter
    return 1000000;
}
