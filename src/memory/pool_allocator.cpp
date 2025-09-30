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

static void* PoolAlloc(Allocator* a, u32 size)
{
    assert(a);

    PoolAllocatorImpl* impl = static_cast<PoolAllocatorImpl*>(a);
    if (impl->count >= impl->capacity)
        return nullptr;

    assert(size == impl->item_size);

    for (u32 i=0; i<impl->capacity; i++)
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

static void* PoolRealloc(Allocator* a, void* ptr, u32 new_size)
{
    (void)a;
    (void)ptr;
    (void)new_size;
    Exit("pool_allocator_realloc not supported");
    return nullptr;
}

static void PoolFree(Allocator* a, void* ptr)
{
    assert(a);
    assert(ptr);
    PoolAllocatorImpl* impl = static_cast<PoolAllocatorImpl*>(a);
    assert(ptr >= impl->items);
    assert(impl->count > 0);

    u32 index = (u32)((u8*)ptr - (u8*)impl->items) / impl->item_size;
    assert(index < impl->capacity);

    impl->items_used[index] = false;
    impl->count--;
}

static void PoolClear(Allocator* a)
{
    assert(a);
    PoolAllocatorImpl* impl = static_cast<PoolAllocatorImpl*>(a);

    memset(impl->items_used, 0, sizeof(bool) * impl->capacity);
    memset(impl->items, 0, impl->item_size * impl->capacity);
    impl->count = 0;
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
    impl->clear = PoolClear;
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
    u32 index = (u32)(corrected_ptr - (u8*)impl->items) / impl->item_size;
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

bool IsValid(PoolAllocator* allocator, u32 index)
{
    assert(allocator);
    assert(index < static_cast<PoolAllocatorImpl*>(allocator)->capacity);
    PoolAllocatorImpl* impl = static_cast<PoolAllocatorImpl*>(allocator);
    return impl->items_used[index];
}

u32 GetCount(PoolAllocator* allocator)
{
    assert(allocator);
    return static_cast<PoolAllocatorImpl*>(allocator)->count;
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

void Enumerate(PoolAllocator* allocator, bool (*func)(u32 index, void* item, void* user_data), void* user_data)
{
    assert(allocator);
    assert(func);
    PoolAllocatorImpl* impl = static_cast<PoolAllocatorImpl*>(allocator);

    for (u32 i=0; i<impl->capacity; i++)
        if (impl->items_used[i])
            if (!func(i, GetAt(allocator, i), user_data))
                break;
}
