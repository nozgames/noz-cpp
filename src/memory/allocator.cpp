//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define GET_ALLOCATOR(a) (a == nullptr ? g_default_allocator : a);

struct AllocHeader
{
    DestructorFunc destructor;
};

// todo: implement debug tracking.
struct DebugAllocHeader
{
    Allocator* allocator;
    size_t size;
};

static void* AllocDefault(Allocator* a, size_t size, DestructorFunc destructor)
{
    auto header = (AllocHeader*)malloc(size + sizeof(AllocHeader));
    header->destructor = destructor;
    return header + 1;
}

static void* ReallocDefault(Allocator* a, void* ptr, size_t new_size)
{
    auto header = ((AllocHeader*)ptr) - 1;
    header = (AllocHeader*)realloc(header, new_size + sizeof(AllocHeader));
    return header + 1;
}

static void FreeDefault(Allocator* a, void* ptr)
{
    auto header = ((AllocHeader*)ptr) - 1;
    auto destructor = header->destructor;
    header->destructor = nullptr;
    if (destructor)
        destructor(ptr);

    free(header);
}

static void PushDefault(Allocator* a)
{
}

static void PopDefault(Allocator* a)
{
}

static void ClearDefault(Allocator* a)
{
}

Allocator g_default_allocator_impl = {
    .alloc = AllocDefault,
    .free = FreeDefault,
    .realloc = ReallocDefault,
    .push = PushDefault,
    .pop = PopDefault,
    .clear = ClearDefault,
    .name = "default"
};

Allocator* g_default_allocator = &g_default_allocator_impl;
Allocator* g_scratch_allocator = nullptr;

void* Alloc(Allocator* a, size_t size, DestructorFunc destructor)
{
    a = GET_ALLOCATOR(a);
    auto ptr = a->alloc(a, size, destructor);

    if (!ptr)
    {
        if (a->stats)
        {
            auto stats = a->stats(a);
            Exit("out_of_memory: %d / %d (%d)", (int)stats.available, (int)stats.total, (int)size);
        }
        else
            Exit("out_of_memory: (%d)", (int)size);
        return nullptr;
    }

    memset(ptr, 0, size);
    return ptr;
}

void Free(Allocator* a, void* ptr)
{
    a = GET_ALLOCATOR(a);
    a->free(a, ptr);
}

void* Realloc(Allocator* a, void* ptr, size_t new_size)
{
    a = GET_ALLOCATOR(a);
    return a->realloc(a, ptr, new_size);
}

void Push(Allocator* a)
{
    a = GET_ALLOCATOR(a);
    a->push(a);
}

void Pop(Allocator* a)
{
    a = GET_ALLOCATOR(a);
    a->pop(a);
}

void Clear(Allocator* a)
{
    a = GET_ALLOCATOR(a);
    a->clear(a);
}

void Destroy(Allocator* a)
{
    if (a == g_default_allocator)
        return;

    free(a);
}

void PushScratch()
{
    Push(g_scratch_allocator);
}

void PopScratch()
{
    Pop(g_scratch_allocator);
}

void ClearScratch()
{
    Clear(g_scratch_allocator);
}

void InitAllocator(ApplicationTraits* traits)
{
    g_scratch_allocator = CreateArenaAllocator(traits->scratch_memory_size, "scratch");
}

void ShutdownAllocator()
{
    Destroy(g_scratch_allocator);
    g_scratch_allocator = nullptr;
}
