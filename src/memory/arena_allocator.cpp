//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// todo: application trait
#define ARENA_ALLOCATOR_MAX_STACK 64

struct ArenaAllocHeader
{
    u32 size;
    u32 reserved;
};

struct ArenaAllocator
{
    Allocator base;
    u8* data;
    size_t* stack;
    size_t stack_depth;
    size_t stack_size;
    size_t stack_overflow;
    size_t size;
    size_t used;
};

void* ArenaAlloc(Allocator* a, size_t size)
{
    auto* impl = (ArenaAllocator*)a;

    // Calculate the total size of the alloc including the alloc header and alignment
    auto total_size = sizeof(ArenaAllocHeader) + size;
    constexpr size_t alignment = sizeof(void*);
    total_size = (total_size + alignment - 1) & ~(alignment - 1);

    // Overflow?
    if (impl->used + total_size > impl->size)
        return nullptr;

    // Reserve the space
    void* ptr = (char*)impl->data + impl->used;
    impl->used += total_size;

    // Stuff a header before the actual data
    auto* header = (ArenaAllocHeader*)ptr;
    header->size = total_size;

    // Return the real data pointer
    return header + 1;
}

void* ArenaRealloc(Allocator* a, void* ptr, size_t new_size)
{
    Exit("arena_allocator_realloc not supported");
    return nullptr;
}

void ArenaFree(Allocator* a, void* ptr)
{
    // todo: if it was the last pointer we can pop it off
}

void ArenaClear(Allocator* a)
{
    auto* impl = (ArenaAllocator*)a;
    assert(impl);

    size_t size = 0;
    while (size < impl->used)
    {
        auto header = (ArenaAllocHeader*)(impl->data + size);
        size += header->size;
    }

    impl->stack[0] = 0;
    impl->stack_depth = 0;
    impl->stack_overflow = 0;
    impl->used = 0;
}


void ArenaPush(Allocator* a)
{
    auto impl = (ArenaAllocator*)a;
    assert(impl);
    if (impl->stack_depth < impl->stack_size)
        impl->stack[impl->stack_depth++] = impl->used;
    else
        impl->stack_overflow++;
}

void ArenaPop(Allocator* a)
{
    auto* impl = (ArenaAllocator*)a;
    assert(impl);
    if (impl->stack_overflow > 0)
        impl->stack_overflow--;
    else if (impl->stack_depth > 0)
        impl->used = impl->stack[--impl->stack_depth];
    else
        // error: stack underflow
        ;
}

AllocatorStats ArenaStats(Allocator* a)
{
    auto* impl = (ArenaAllocator*)a;
    assert(impl);

    return { impl->size, impl->used };
}

Allocator* CreateArenaAllocator(size_t size, const char* name)
{
    auto* allocator = (ArenaAllocator*)calloc(
        1,
        sizeof(ArenaAllocator) +
        size +
        sizeof(size_t) * ARENA_ALLOCATOR_MAX_STACK);

    if (!allocator)
        return nullptr;

    allocator->base = {
        .alloc = ArenaAlloc,
        .free = ArenaFree,
        .realloc = ArenaRealloc,
        .push = ArenaPush,
        .pop = ArenaPop,
        .clear = ArenaClear,
        .stats = ArenaStats,
        .name = name,
    };
    allocator->stack = (size_t*)(allocator + 1);
    allocator->stack_size = ARENA_ALLOCATOR_MAX_STACK;
    allocator->size = size;
    allocator->data = (u8*)(allocator->stack + ARENA_ALLOCATOR_MAX_STACK);
    return (Allocator*)allocator;
}

