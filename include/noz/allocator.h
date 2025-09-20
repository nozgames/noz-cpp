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
    void* (*alloc)(Allocator*, u32 size);
    void (*free)(Allocator*, void* ptr);
    void* (*realloc)(Allocator*, void* ptr, u32 new_size);
    void (*push)(Allocator*);
    void (*pop)(Allocator*);
    void (*clear)(Allocator*);
    AllocatorStats (*stats)(Allocator*);
    const char* name;
};

struct AllocHeader
{
    DestructorFunc destructor;
    Allocator* allocator;
#ifdef _DEBUG
    u64 checksum;
#endif
};

void* Alloc(Allocator* a, u32 size, DestructorFunc = nullptr);
void Free(void* ptr);
void* Realloc(void* ptr, u32 new_size);
void Push(Allocator* a);
void Pop(Allocator* a);
void Clear(Allocator* a);
void Destroy(Allocator* a);
Allocator* GetAllocator(void* ptr);

// @arena
Allocator* CreateArenaAllocator(u32 size, const char* name);

// @pool
struct PoolAllocator : Allocator { };

extern PoolAllocator* CreatePoolAllocator(u32 item_size, u32 capacity);
extern void* GetAt(PoolAllocator* allocator, u32 index);
extern u32 GetIndex(PoolAllocator* allocator, const void* ptr);
extern bool IsFull(PoolAllocator* allocator);
extern bool IsEmpty(PoolAllocator* allocator);
extern u32 GetCount(PoolAllocator* allocator);

// @scratch
extern void PushScratch();
extern void PopScratch();
extern void ClearScratch();

extern Allocator* g_default_allocator;
extern Allocator* g_scratch_allocator;

#define ALLOCATOR_DEFAULT   g_default_allocator
#define ALLOCATOR_SCRATCH   g_scratch_allocator
