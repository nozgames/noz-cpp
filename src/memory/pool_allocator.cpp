//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct PoolAllocatorImpl : PoolAllocator
{
    Allocator base;
    u32* free_list;
    u32 count;
    u32 capacity;
    u32 item_size;
    void* items;
};

void* PoolAlloc(Allocator* a, size_t size)
{
    assert(a);

    PoolAllocatorImpl* impl = static_cast<PoolAllocatorImpl*>(a);
    if (impl->count >= impl->capacity)
        return nullptr;

    assert((u32)size == impl->item_size);

    u32 index = impl->free_list[impl->count];
    impl->count++;
    return GetAt(impl, index);
}

void* PoolRealloc(Allocator* aa, void* ptr, size_t new_size)
{
    Exit("pool_allocator_realloc not supported");
    return nullptr;
}

void PoolFree(Allocator* a, void* ptr)
{
    assert(a);
    assert(ptr);
    PoolAllocatorImpl* impl = static_cast<PoolAllocatorImpl*>(a);
    assert(ptr >= impl->items);

    u32 index = ((u8*)ptr - (u8*)impl->items) / impl->item_size;
    assert(index < impl->capacity);

    impl->count--;
    impl->free_list[impl->count] = index;
}

PoolAllocator* CreatePoolAllocator(u32 item_size, u32 capacity)
{
    u32 alloc_size = sizeof(PoolAllocatorImpl) + (item_size + sizeof(u32)) * capacity;
    PoolAllocatorImpl* impl = (PoolAllocatorImpl*)calloc(1, alloc_size);
    impl->alloc = PoolAlloc;
    impl->free = PoolFree;
    impl->realloc = PoolRealloc;
    impl->free_list = (u32*)(impl + 1);
    impl->items = impl->free_list + capacity;
    impl->capacity = capacity;
    impl->item_size = item_size;

    u32* f = impl->free_list;
    for (u32 i=0; i<capacity; i++, f++)
        *f = i;

    return impl;
}

void* GetAt(PoolAllocator* allocator, u32 index)
{
    assert(allocator);
    PoolAllocatorImpl* impl = static_cast<PoolAllocatorImpl*>(allocator);
    assert(index < impl->capacity);
    return (u8*)impl->items + impl->item_size * index;
}
