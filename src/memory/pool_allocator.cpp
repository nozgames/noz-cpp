//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct PoolEntry
{
    PoolEntry* next;
};

struct PoolAllocator
{
    Allocator base;
    PoolEntry* entries;
    PoolEntry* free;
    size_t count;
};

void* PoolAlloc(Allocator* aptr, size_t size)
{
    auto a = (PoolAllocator*)aptr;
    assert(a);
    if (!a->free)
        return nullptr;

    PoolEntry* entry = a->free;
    a->free = entry->next;
    a->count++;
    return entry + 1;
}

void* PoolRealloc(Allocator* aa, void* ptr, size_t new_size)
{
    Exit("pool_allocator_realloc not supported");
    return nullptr;
}

void PoolFree(Allocator* aa, void* ptr)
{
    auto a = (PoolAllocator*)aa;
    assert(a);
    if (!ptr)
        return;
    auto entry = (PoolEntry*)((char*)ptr - sizeof(PoolEntry));
    entry->next = a->free;
    a->count--;
    a->free = entry;
}

Allocator* CreatePoolAllocator(size_t entry_size, size_t entry_count)
{
    size_t stride = entry_size + sizeof(PoolEntry);
    auto* a = (PoolAllocator*)calloc(1, sizeof(PoolAllocator) + stride * entry_count);
    a->base = {
        .alloc = PoolAlloc,
        .free = PoolFree,
        .realloc = PoolRealloc
    };
    a->entries = (PoolEntry*)(a + 1);

    // link all entries into a free list
    PoolEntry* prev = a->free = a->entries;
    for (size_t i = 1; i < entry_count - 1; i++)
    {
        auto* entry = (PoolEntry*)((char*)a->entries + i * stride);
        prev->next = entry;
        prev = entry;
    }

    return (Allocator*)a;
}
