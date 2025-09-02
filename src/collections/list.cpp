//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define DEFAULT_CAPACITY 32

struct ListImpl : List
{
    void* items;
    u32 count;
    u32 item_size;
    u32 capacity;
};

void ListDestructor(void* ptr)
{
    ListImpl* impl = static_cast<ListImpl*>(ptr);
    Free(impl->items);
}

List* CreateList(Allocator* allocator, size_t item_size, size_t capacity)
{   
    if (capacity == 0)
	capacity = DEFAULT_CAPACITY;

    List* list = (List*)Alloc(allocator, sizeof(ListImpl), ListDestructor);
    ListImpl* impl = static_cast<ListImpl*>(list);
    if (!list)
        return nullptr;
    
    impl->count = 0;
    impl->capacity = capacity;
    impl->items = Alloc(allocator, item_size * capacity);
    if (!impl->items)
    {
        Free(list);
        return nullptr;
    }
    
    return list;
}

u32 GetCount(List* list)
{
    return static_cast<ListImpl*>(list)->count;
}

bool IsFull(List* list)
{
    assert(list);
    ListImpl* impl = static_cast<ListImpl*>(list);
    return impl->count == impl->capacity;
}

void* GetAt(List* list, u32 index)
{
    assert(list);
    ListImpl* impl = static_cast<ListImpl*>(list);
    assert(index < impl->count);
    return (u8*)impl->items + index * impl->item_size;
}

static void* AddInternal(List* list)
{
    assert(list);

    if (IsFull(list))
        return nullptr;

    ListImpl* impl = static_cast<ListImpl*>(list);
    impl->count++;
    return GetAt(list, impl->count - 1);
}

void* Add(List* list)
{
    assert(list);

    void* item = AddInternal(list);
    if (!item)
        return nullptr;

    ListImpl* impl = static_cast<ListImpl*>(list);
    memset(item, 0, impl->item_size);
    return item;
}

void* Add(List* list, const void* value)
{
    assert(list);
    assert(value);

    void* item = AddInternal(list);
    if (!item)
        return nullptr;

    ListImpl* impl = static_cast<ListImpl*>(list);
    memcpy(item, value, impl->item_size);
    return item;
}

void Remove(List* list, const void* item)
{
    assert(list);
    assert(item);
    RemoveRange(list, item, 1);
}

void RemoveRange(List* list, const void* first, int count)
{
    assert(list);
    assert(first);
    assert(count > 0);

    ListImpl* impl = static_cast<ListImpl*>(list);
    assert(first >= impl->items);

    u32 index = ((u8*)first - (u8*)impl->items) / impl->item_size;
    assert(index < impl->count);

    if (index + count < impl->count)
        memmove(GetAt(list, index), GetAt(list, index + count), impl->item_size * (impl->count - index - count));

    impl->count -= count;
}

void RemoveAt(List* list, u32 index)
{
    assert(list);
    ListImpl* impl = static_cast<ListImpl*>(list);
    assert(index < impl->count);
    RemoveRange(list, GetAt(list, index), 1);
}

void Clear(List* list)
{
    assert(list);
    ListImpl* impl = static_cast<ListImpl*>(list);
    impl->count = 0;
}
