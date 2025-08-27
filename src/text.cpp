//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <cctype>

void SetValue(text_t& dst, const text_t& src)
{
    dst.length = src.length;
    memcpy(dst.value, src.value, src.length + 1);
}

void SetValue(text_t& dst, const char* src)
{
    assert(src);

    size_t src_len = strlen(src);
    if (src_len == 0)
    {
        Clear(dst);
        return;
    }

    dst.length = min(src_len, TEXT_SIZE - 1);
    memcpy(dst.value, src, src_len + 1);
    dst.length = src_len;
}

void Append(text_t& dst, const char* src)
{
    assert(src);

    size_t src_len = min(strlen(src), TEXT_SIZE - dst.length - 1);
    if (src_len == 0)
        return;

    memcpy(dst.value + dst.length, src, src_len + 1);
    dst.length += src_len;
}

void text_format(text_t& dst, const char* fmt, ...)
{
    assert(fmt);
    
    va_list args;
    va_start(args, fmt);
    
    int written = vsnprintf(dst.value, TEXT_SIZE - 1, fmt, args);
    va_end(args);
    
    if (written < 0)
    {
        Clear(dst);
        return;
    }

    dst.value[written] = 0;
}

void Trim(text_t& text)
{
    if (text.length == 0)
        return;

    // Trim leading whitespace
    size_t start = 0;
    while (start < text.length && isspace((unsigned char)text.value[start]))
        start++;

    // All whitespace?
    if (start >= text.length)
    {
        Clear(text);
        return;
    }

    // Trim trailing whitespace
    size_t end = text.length;
    while (end > start && isspace((unsigned char)text.value[end - 1]))
        end--;

    // Move content to beginning if needed
    if (start > 0)
        memmove(text.value, text.value + start, end - start);

    text.length = end - start;
    text.value[text.length] = '\0';
}

bool Equals(const text_t& a, const text_t& b)
{
    if (a.length != b.length)
        return false;

    return memcmp(a.value, b.value, a.length) == 0;
}

bool Equals(const text_t& text, const char* str)
{
    assert(str);
    
    size_t str_len = strlen(str);
    if (text.length != str_len)
        return false;

    return memcmp(text.value, str, str_len) == 0;
}
