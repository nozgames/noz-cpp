//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <cstdarg>
#include <cstdio>

void Copy(char* dst, u32 dst_size, const char* src)
{
    assert(dst);
    assert(src);

    if (dst_size == 0)
        return;

    u32 i=0;
    for (i=0; i<dst_size-1 && src[i]; i++)
        dst[i] = src[i];

    dst[i] = 0;
}

u32 Length(const char* str)
{
    if (!str)
        return 0;

    u32 len = 0;
    while (str[len])
        len++;

    return len;
}

void Format(char* dst, u32 dst_size, const char* fmt, ...)
{
    assert(dst);
    assert(fmt);

    if (dst_size == 0)
        return;

    va_list args;
    va_start(args, fmt);
    int result = std::vsnprintf(dst, dst_size, fmt, args);
    va_end(args);

    if (result < 0 || (u32)result >= dst_size)
        dst[dst_size-1] = 0;
}

void Format(char* dst, u32 dst_size, const char* fmt, va_list args)
{
    assert(dst);
    assert(fmt);

    if (dst_size == 0)
        return;

    int result = std::vsnprintf(dst, dst_size, fmt, args);

    if (result < 0 || (u32)result >= dst_size)
        dst[dst_size-1] = 0;
}