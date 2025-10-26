//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//


RingBuffer* CreateRingBuffer(Allocator* allocator, u32 item_size, u32 capacity)
{
    auto rb = (RingBuffer*)Alloc(allocator, sizeof(RingBuffer));
    rb->item_size = item_size;
    rb->capacity = capacity;
    rb->count = 0;
    rb->front = 0;
    rb->back = 0;
    rb->items = (u8*)Alloc(allocator, item_size * capacity);
    return rb;
}

void* PushFront(RingBuffer* rb)
{
    if (rb->count == rb->capacity)
        return nullptr;

    rb->count++;
    rb->front = (rb->front + rb->capacity - 1) % rb->capacity;
    return rb->items + rb->front * rb->item_size;
}

void* PushFront(RingBuffer* rb, const void* value)
{
    auto item = PushFront(rb);
    if (!item)
        return nullptr;
    memcpy(item, value, rb->item_size);
    return item;
}

void* PushBack(RingBuffer* rb)
{
    if (rb->count == rb->capacity)
        return nullptr;

    rb->count++;
    rb->back = (rb->back + 1) % rb->capacity;
    return GetAt(rb, rb->count - 1);
}

void* PushBack(RingBuffer* rb, const void* value)
{
    auto item = PushBack(rb);
    if (!item)
        return nullptr;
    memcpy(item, value, rb->item_size);
    return item;
}

void PopFront(RingBuffer* rb)
{
    if (rb->count == 0)
        return;

    rb->count--;
    rb->front = (rb->front + 1) % rb->capacity;
}

void PopBack(RingBuffer* rb)
{
    if (rb->count == 0)
        return;

    rb->count--;
    rb->back = (rb->back + 1) % rb->capacity;
}

void RemoveAt(RingBuffer* rb, u32 index) {
    assert(index < rb->count);
    for (u32 i=index; i<rb->count-1; i++)
        memcpy(GetAt(rb, i), GetAt(rb, i+1), rb->item_size);
    rb->count--;
}

void Clear(RingBuffer* rb)
{
    rb->count = 0;
    rb->front = 0;
    rb->back = 0;
}

void* GetAt(RingBuffer* rb, u32 index)
{
    assert(index < rb->count);
    return rb->items + ((rb->front + index) % rb->capacity) * rb->item_size;
}