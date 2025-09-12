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

void Copy(char* dst, u32 dst_size, const char* src, u32 length)
{
    assert(dst);
    assert(src);

    if (dst_size == 0)
        return;

    u32 i=0;
    for (i=0; i<dst_size-1 && src[i] && length > 0; i++, length--)
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

bool Equals(const char* s1, const char* s2, u32 length, bool ignore_case)
{
    if (s1 == s2)
        return true;

    if (!s1 || !s2)
        return false;

    for ( ; *s1 && *s2 && length > 0; length--, s1++, s2++)
    {
        char c1 = *s1;
        char c2 = *s2;

        if (ignore_case)
        {
            if (c1 >= 'A' && c1 <= 'Z')
                c1 = (char)(c1 - 'A' + 'a');

            if (c2 >= 'A' && c2 <= 'Z')
                c2 = (char)(c2 - 'A' + 'a');
        }

        if (c1 != c2)
            return false;
    }

    return length == 0 || *s1 == *s2;
}

bool Equals(const char* s1, const char* s2, bool ignore_case)
{
    return Equals(s1, s2, U32_MAX, ignore_case);
}

char* CleanPath(char* path)
{
    assert(path);

    for (char* s = path; *s ; s++)
        if (*s == '\\')
            *s = '/';

    return path;
}
