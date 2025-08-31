//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//


struct RingBufferImpl : Object
{
    u8* items;
    u32 item_size;
    u32 capacity;
    u32 count;
    u32 front;
    u32 back;
};

static RingBufferImpl* Impl(RingBuffer* l) { return (RingBufferImpl*)Cast(l, TYPE_RING_BUFFER); }

RingBuffer* CreateRingBuffer(Allocator* allocator, u32 item_size, u32 capacity)
{
    auto rb = (RingBuffer*)CreateObject(allocator, sizeof(RingBufferImpl), TYPE_RING_BUFFER);
    auto impl = Impl(rb);
    impl->item_size = item_size;
    impl->capacity = capacity;
    impl->count = 0;
    impl->front = 0;
    impl->back = 0;
    impl->items = (u8*)Alloc(allocator, item_size * capacity);
    return rb;
}

void* PushFront(RingBuffer* rb)
{
    auto impl = Impl(rb);
    if (impl->count == impl->capacity)
        return nullptr;

    impl->count++;
    impl->front = (impl->front + impl->capacity - 1) % impl->capacity;
    return impl->items + impl->front * impl->item_size;
}

void* PushFront(RingBuffer* buffer, const void* value)
{
    auto impl = Impl(buffer);
    auto item = PushFront(buffer);
    if (!item)
        return nullptr;
    memcpy(item, value, impl->item_size);
    return item;
}

void* PushBack(RingBuffer* rb)
{
    auto impl = Impl(rb);
    if (impl->count == impl->capacity)
        return nullptr;

    impl->count++;
    impl->back = (impl->back + 1) % impl->capacity;
    return GetAt(rb, impl->back);
}

void* PushBack(RingBuffer* rb, const void* value)
{
    auto impl = Impl(rb);
    auto item = PushBack(rb);
    if (!item)
        return nullptr;
    memcpy(item, value, impl->item_size);
    return item;
}

void PopFront(RingBuffer* rb)
{
    auto impl = Impl(rb);
    if (impl->count == 0)
        return;

    impl->count--;
    impl->front = (impl->front + 1) % impl->capacity;
}

void PopBack(RingBuffer* buffer)
{
    auto impl = Impl(buffer);
    if (impl->count == 0)
        return;

    impl->count--;
    impl->back = (impl->back + 1) % impl->capacity;
}

u32 GetCount(RingBuffer* buffer)
{
    return Impl(buffer)->count;
}

void Clear(RingBuffer* buffer)
{
    auto impl = Impl(buffer);
    impl->count = 0;
    impl->front = 0;
    impl->back = 0;
}

void* GetAt(RingBuffer* list, u32 index)
{
    auto impl = Impl(list);
    assert(index < impl->count);
    return impl->items + (impl->front + index) * impl->item_size;
}

bool IsEmpty(RingBuffer* list)
{
    return Impl(list)->count == 0;
}

bool IsFull(RingBuffer* list)
{
    return Impl(list)->count == Impl(list)->capacity;
}

