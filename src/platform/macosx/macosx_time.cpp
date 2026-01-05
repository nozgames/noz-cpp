//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../../platform.h"
#include <mach/mach_time.h>

u64 GetPerformanceCounter()
{
    return mach_absolute_time();
}

u64 GetPerformanceFrequency()
{
    static u64 frequency = 0;
    if (frequency == 0)
    {
        mach_timebase_info_data_t timebase;
        mach_timebase_info(&timebase);
        // Convert to nanoseconds per second (1e9)
        frequency = (1000000000ULL * timebase.denom) / timebase.numer;
    }
    return frequency;
}
