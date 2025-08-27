//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Map
{
    size_t capacity;
    size_t count;
    u64* keys;
    void* values;
    size_t value_stride;
};

inline void Init(Map& map, u64* keys, void* values, size_t capacity, size_t value_stride, size_t initial_count=0)
{
    map.capacity = capacity;
    map.keys = keys;
    map.values = values;
    map.value_stride = value_stride;
    map.count = initial_count;
}

bool HasKey(const Map& map, u64 key);
void* GetValue(const Map& map, const char* key);
void* GetValue(const Map& map, u64 key);
void* SetValue(Map& map, const char* key, void* value = nullptr);
void* SetValue(Map& map, u64 key, void* value = nullptr);
