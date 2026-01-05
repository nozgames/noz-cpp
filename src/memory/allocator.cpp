//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#define GET_ALLOCATOR(a) (a == nullptr ? g_default_allocator : a);

static AllocHeader* GetHeader(void* p) { return (AllocHeader*)p - 1; }

#ifdef _DEBUG
u64 CreateChecksum(Allocator* a, void* ptr)
{
    return (u64)ptr ^ (u64)a;
}

void ValidateHeader(void *p)
{
    AllocHeader* header = GetHeader(p);
    u64 checksum = CreateChecksum(header->allocator, p);
    assert(checksum == header->checksum);
}
#endif

static void* AllocDefault(Allocator* a, u32 size) {
    (void)a;
    return malloc(size);
}

static void* ReallocDefault(Allocator* a, void* ptr, u32 new_size) {
    (void)a;
    return realloc(ptr, new_size);
}

static void FreeDefault(Allocator* a, void* ptr)
{
    (void)a;
    free(ptr);
}

static void PushDefault(Allocator* a)
{
    (void)a;
}

static void PopDefault(Allocator* a)
{
    (void)a;
}

static void ClearDefault(Allocator* a)
{
    (void)a;
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

void* Alloc(Allocator* a, u32 size, DestructorFunc destructor)
{
    a = GET_ALLOCATOR(a);

    AllocHeader* header = (AllocHeader*)a->alloc(a, size + sizeof(AllocHeader));
    if (!header)
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

    void* ptr = header + 1;

    header->destructor = destructor;
    header->allocator = a;

#ifdef _DEBUG
    header->checksum = CreateChecksum(a, ptr);
#endif

    memset(ptr, 0, size);
    return ptr;
}

void Free(void* ptr)
{
    if (ptr == nullptr)
        return;

#if _DEBUG
    ValidateHeader(ptr);
#endif

    AllocHeader* header = GetHeader(ptr);
    Allocator* a = header->allocator;

    if (header->destructor)
        header->destructor(ptr);

    a->free(a, header);
}

void* Realloc(void* ptr, u32 new_size)
{
#if _DEBUG
    ValidateHeader(ptr);
#endif

    AllocHeader* header = GetHeader(ptr);
    Allocator* a = header->allocator;
    a = GET_ALLOCATOR(a);
    header = (AllocHeader*)a->realloc(a, header, new_size + sizeof(AllocHeader));
    ptr = (void*)(header + 1);
#ifdef _DEBUG
    header->checksum = CreateChecksum(a, ptr);
    ValidateHeader(ptr);
#endif

    return ptr;
}

Allocator* GetAllocator(void* ptr)
{
#if _DEBUG
    ValidateHeader(ptr);
#endif

    return GetHeader(ptr)->allocator;
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
