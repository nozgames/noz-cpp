//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct PoolAllocatorImpl : PoolAllocator
{
    Allocator base;
    u32 count;
    u32 capacity;
    u32 item_size;
    bool* items_used;
    void* items;
};

void* PoolAlloc(Allocator* a, size_t size)
{
    assert(a);

    PoolAllocatorImpl* impl = static_cast<PoolAllocatorImpl*>(a);
    if (impl->count >= impl->capacity)
        return nullptr;

    assert((u32)size == impl->item_size);

    for (int i=0; i<impl->capacity; i++)
    {
        if (!impl->items_used[i])
        {
            impl->items_used[i] = true;
            impl->count++;
            return (u8*)impl->items + impl->item_size * i;
        }
    }

    return nullptr;
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
    assert(impl->count > 0);

    u32 index = ((u8*)ptr - (u8*)impl->items) / impl->item_size;
    assert(index < impl->capacity);

    impl->items_used[index] = false;
    impl->count--;
}

PoolAllocator* CreatePoolAllocator(u32 item_size, u32 capacity)
{
    // The Alloc function will request extra space for the AllocHeader so prepare for it.
    item_size += sizeof(AllocHeader);

    u32 alloc_size = sizeof(PoolAllocatorImpl) + (item_size + sizeof(bool)) * capacity;
    PoolAllocatorImpl* impl = (PoolAllocatorImpl*)calloc(1, alloc_size);
    impl->alloc = PoolAlloc;
    impl->free = PoolFree;
    impl->realloc = PoolRealloc;
    impl->items_used = (bool*)(impl + 1);
    impl->items = impl->items_used + capacity;
    impl->capacity = capacity;
    impl->item_size = item_size;
    return impl;
}

u32 GetIndex(PoolAllocator* allocator, const void* ptr)
{
    assert(allocator);
    PoolAllocatorImpl* impl = static_cast<PoolAllocatorImpl*>(allocator);

    u8* corrected_ptr = (u8*)ptr - sizeof(AllocHeader);
    assert(corrected_ptr >= impl->items);
    u32 index = (corrected_ptr - (u8*)impl->items) / impl->item_size;
    assert(index < impl->capacity);
    return index;
}

void* GetAt(PoolAllocator* allocator, u32 index)
{
    assert(allocator);
    PoolAllocatorImpl* impl = static_cast<PoolAllocatorImpl*>(allocator);
    assert(index < impl->capacity);
    return (u8*)impl->items + impl->item_size * index + sizeof(AllocHeader);
}

bool IsFull(PoolAllocator* allocator)
{
    assert(allocator);
    PoolAllocatorImpl* impl = static_cast<PoolAllocatorImpl*>(allocator);
    return impl->count == impl->capacity;
}

bool IsEmpty(PoolAllocator* allocator)
{
    assert(allocator);
    PoolAllocatorImpl* impl = static_cast<PoolAllocatorImpl*>(allocator);
    return impl->count == 0;
}
