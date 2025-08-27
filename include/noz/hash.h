//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

u64 Hash(void* data, size_t size);
u64 Hash(const char* str);
inline u64 Hash(const name_t* name) { return (u64)name; }

#define hash_combine(...) Hash(__VA_ARGS__, 0)

u64 Hash(void* data, size_t size, u64 seed);

static u64 Hash(u64 h1, u64 h2)
{
    u64 result = h1;
    if (h2) result = Hash(&h2, sizeof(h2), result);
    return result;
}

static u64 Hash(u64 h1, u64 h2, u64 h3)
{
    u64 result = h1;
    if (h2) result = Hash(&h2, sizeof(h2), result);
    if (h3) result = Hash(&h3, sizeof(h3), result);
    return result;
}