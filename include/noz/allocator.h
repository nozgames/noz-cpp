//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
#pragma once

typedef void (*DestructorFunc)(void*);

struct AllocatorStats
{
    size_t total;
    size_t available;
};

// @allocator
struct Allocator
{
    void* (*alloc)(Allocator*, size_t size);
    void (*free)(Allocator*, void* ptr);
    void* (*realloc)(Allocator*, void* ptr, size_t new_size);
    void (*push)(Allocator*);
    void (*pop)(Allocator*);
    void (*clear)(Allocator*);
    AllocatorStats (*stats)(Allocator*);
    const char* name;
};

void* Alloc(Allocator* a, size_t size, DestructorFunc = nullptr);
void Free(void* ptr);
void* Realloc(void* ptr, size_t new_size);
void Push(Allocator* a);
void Pop(Allocator* a);
void Clear(Allocator* a);
void Destroy(Allocator* a);
Allocator* GetAllocator(void* ptr);

// @arena
Allocator* CreateArenaAllocator(size_t size, const char* name);

// @pool
Allocator* CreatePoolAllocator(size_t entry_size, size_t entry_count);

// @scratch
void PushScratch();
void PopScratch();
void ClearScratch();

extern Allocator* g_default_allocator;
extern Allocator* g_scratch_allocator;

#define ALLOCATOR_DEFAULT   g_default_allocator
#define ALLOCATOR_SCRATCH   g_scratch_allocator
