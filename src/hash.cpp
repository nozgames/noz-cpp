//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <string.h>
#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#include <xxhash.h>

u64 Hash(const void* data, size_t size)
{
    return XXH64(data, size, 0);
}

u64 Hash(const char* str) 
{
    if (!str) return 0;
    return XXH64(str, strlen(str), 0);
}

u64 Hash(void* data, size_t size, u64 seed) 
{
    return XXH64(data, size, seed);
}

u64 Hash(const text_t& text)
{
    return XXH64(text.value, text.length, 0);
}
