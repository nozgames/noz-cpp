//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <cctype>
#include <cstdarg>
#include <cstdio>

void SetValue(Text& dst, const Text& src)
{
    dst.length = src.length;
    memcpy(dst.value, src.value, src.length + 1);
}

void SetValue(Text& text, const char* value, int length) {
    assert(value);
    if (length == 0 || *value == 0) {
        Clear(text);
        return;
    }

    text.length = Min(length, TEXT_MAX_LENGTH - 1);
    memcpy(text.value, value, text.length);
    text.value[text.length] = 0;
}

void SetValue(Text& dst, const char* src) {
    assert(src);

    u32 src_len = Length(src);
    if (src_len == 0)
    {
        Clear(dst);
        return;
    }

    dst.length = Min(src_len, (u32)(TEXT_MAX_LENGTH - 1));
    memcpy(dst.value, src, src_len + 1);
    dst.length = src_len;
}

void Append(Text& dst, const char* src)
{
    assert(src);

    u32 src_len = Min((u32)strlen(src), (u32)(TEXT_MAX_LENGTH - dst.length - 1));
    if (src_len == 0)
        return;

    memcpy(dst.value + dst.length, src, src_len + 1);
    dst.length += src_len;
}

void Trim(Text& text)
{
    if (text.length == 0)
        return;

    // Trim leading whitespace
    int start = 0;
    while (start < text.length && isspace(static_cast<unsigned char>(text.value[start])))
        start++;

    // All whitespace?
    if (start >= text.length)
    {
        Clear(text);
        return;
    }

    // Trim trailing whitespace
    int end = text.length;
    while (end > start && isspace((unsigned char)text.value[end - 1]))
        end--;

    // Move content to beginning if needed
    if (start > 0)
        memmove(text.value, text.value + start, end - start);

    text.length = end - start;
    text.value[text.length] = '\0';
}

bool Equals(const Text& a, const Text& b)
{
    if (a.length != b.length)
        return false;

    return memcmp(a.value, b.value, a.length) == 0;
}

bool Equals(const Text& text, const char* str)
{
    assert(str);
    
    size_t str_len = strlen(str);
    if (text.length != str_len)
        return false;

    return memcmp(text.value, str, str_len) == 0;
}
