//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

// todo: application trait
#define ARENA_ALLOCATOR_MAX_STACK 64

namespace noz {

    struct ArenaAllocHeader {
        u32 size;
        u32 reserved;
    };

    struct ArenaAllocator {
        Allocator base;
        u8* data;
        u32* stack;
        u32 stack_depth;
        u32 stack_size;
        u32 stack_overflow;
        u32 size;
        u32 used;
    };

    static void* ArenaAlloc(Allocator* a, u32 size)
    {
        auto* impl = (ArenaAllocator*)a;

        // Calculate the total size of the alloc including the alloc header and alignment
        u32 total_size = (u32)sizeof(ArenaAllocHeader) + size;
        constexpr u32 alignment = sizeof(void*);
        total_size = (total_size + alignment - 1) & ~(alignment - 1);

        // Overflow?
        if (impl->used + total_size > impl->size)
            return nullptr;

        // Reserve the space
        u8* ptr = impl->data + impl->used;
        impl->used += total_size;

        // Stuff a header before the actual data
        ArenaAllocHeader* header = (ArenaAllocHeader*)ptr;
        header->size = total_size;

        // Return the real data pointer
        return header + 1;
    }

    static void* ArenaRealloc(Allocator* a, void* ptr, u32 new_size)
    {
        (void)a;
        (void)ptr;
        (void)new_size;
        Exit("arena_allocator_realloc not supported");
        return nullptr;
    }

    static void ArenaFree(Allocator* a, void* ptr)
    {
        (void)a;
        (void)ptr;
    }

    static void ArenaClear(Allocator* a)
    {
        auto* impl = (ArenaAllocator*)a;
        assert(impl);

        u32 size = 0;
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

    static void ArenaPush(Allocator* a)
    {
        auto impl = (ArenaAllocator*)a;
        assert(impl);
        if (impl->stack_depth < impl->stack_size)
            impl->stack[impl->stack_depth++] = impl->used;
        else
            impl->stack_overflow++;
    }

    static void ArenaPop(Allocator* a)
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

    Allocator* CreateArenaAllocator(u32 size, const char* name)
    {
        auto* allocator = (ArenaAllocator*)calloc(
            1,
            sizeof(ArenaAllocator) +
            size +
            sizeof(u32) * ARENA_ALLOCATOR_MAX_STACK);

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
        allocator->stack = (u32*)(allocator + 1);
        allocator->stack_size = ARENA_ALLOCATOR_MAX_STACK;
        allocator->size = size;
        allocator->data = (u8*)(allocator->stack + ARENA_ALLOCATOR_MAX_STACK);
        return (Allocator*)allocator;
    }
}
