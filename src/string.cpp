//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <cstdarg>
#include <cstdio>
#include "string.h"

int Copy(char* dst, int dst_size, const char* src) {
    assert(dst);
    if (dst_size == 0)
        return 0;

    if (!src) {
        dst[0] = 0;
        return 0;
    }

    int i=0;
    for (i=0; i<dst_size-1 && src[i]; i++)
        dst[i] = src[i];

    dst[i] = 0;

    return i;
}

int Copy(char* dst, int dst_size, const char* src, int length) {
    assert(dst);
    assert(src);

    if (dst_size == 0)
        return 0;

    int i=0;
    for (i=0; i<dst_size-1 && src[i] && length > 0; i++, length--)
        dst[i] = src[i];

    dst[i] = 0;

    return i;
}

int Length(const char* str) {
    if (!str)
        return 0;

    int len = 0;
    while (str[len])
        len++;

    return len;
}

static int FormatInternal(char* dst, u32 dst_size, const char* fmt, va_list args) {
    assert(dst);
    assert(fmt);

    if (dst_size == 0)
        return 0;

    int result = std::vsnprintf(dst, dst_size, fmt, args);

    if (result < 0 || static_cast<u32>(result) >= dst_size) {
        dst[dst_size-1] = 0;
        result = 0;
    }

    return result;
}

int Format (String32& dst, const char* fmt, ...) {
    assert(fmt);
    va_list args;
    va_start(args, fmt);
    dst.length = FormatInternal(dst.value, sizeof(dst.value), fmt, args);
    va_end(args);
    return dst.length;
}

int Format (String64& dst, const char* fmt, ...) {
    assert(fmt);
    va_list args;
    va_start(args, fmt);
    dst.length = FormatInternal(dst.value, sizeof(dst.value), fmt, args);
    va_end(args);
    return dst.length;
}

int Format (String128& dst, const char* fmt, ...) {
    assert(fmt);
    va_list args;
    va_start(args, fmt);
    dst.length = FormatInternal(dst.value, sizeof(dst.value), fmt, args);
    va_end(args);
    return dst.length;
}
int Format (String256& dst, const char* fmt, ...) {
    assert(fmt);
    va_list args;
    va_start(args, fmt);
    dst.length = FormatInternal(dst.value, sizeof(dst.value), fmt, args);
    va_end(args);
    return dst.length;
}
int Format (String1024& dst, const char* fmt, ...) {
    assert(fmt);
    va_list args;
    va_start(args, fmt);
    dst.length = FormatInternal(dst.value, sizeof(dst.value), fmt, args);
    va_end(args);
    return dst.length;
}

int Format (String4096& dst, const char* fmt, ...) {
    assert(fmt);
    va_list args;
    va_start(args, fmt);
    dst.length = FormatInternal(dst.value, sizeof(dst.value), fmt, args);
    va_end(args);
    return dst.length;
}

int Format(char* dst, u32 dst_size, const char* fmt, ...) {
    assert(fmt);
    va_list args;
    va_start(args, fmt);
    int result = FormatInternal(dst, dst_size, fmt, args);
    va_end(args);
    return result;
}

int Format(char* dst, u32 dst_size, const char* fmt, va_list args) {
    return FormatInternal(dst, dst_size, fmt, args);
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

bool Contains(const char* s1, const char* s2, bool ignore_case) {
    assert(s1);
    assert(s2);

    u32 len2 = Length(s2);
    if (len2 == 0)
        return true;

    u32 len1 = Length(s1);
    if (len1 < len2)
        return false;

    u32 count = len1 - len2 + 1;
    for (u32 i=0; i<count; i++, s1++) {
        if (Equals(s1, s2, len2, ignore_case))
            return true;
    }

    return false;
}

char* CleanPath(char* path)
{
    assert(path);

    for (char* s = path; *s ; s++)
        if (*s == '\\')
            *s = '/';

    return path;
}

int Compare(const char* s1, const char* s2, bool ignore_case)
{
    if (s1 == s2)
        return 0;

    if (ignore_case)
#if defined(WIN32)
        return _stricmp(s1, s2);
#else
        return strcasecmp(s1, s2);
#endif

    return strcmp(s1, s2);
}

void Lower(char* dst, u32 dst_size) {
    for (;*dst && dst_size > 0; dst++, dst_size--)
        if (*dst >= 'A' && *dst <= 'Z')
            *dst = (char)(*dst - 'A' + 'a');
}

void Upper(char* dst, u32 dst_size) {
    for (;*dst && dst_size > 0; dst++, dst_size--)
        if (*dst >= 'a' && *dst <= 'z')
            *dst = (char)(*dst - 'a' + 'A');
}

void Replace(char* dst, u32 dst_size, char find, char replace) {
    for (;*dst && dst_size > 0; dst++, dst_size--)
        if (*dst == find)
            *dst = replace;
}
