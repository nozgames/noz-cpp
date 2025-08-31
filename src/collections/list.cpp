//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define DEFAULT_CAPACITY 32

struct ListImpl : Object
{
    size_t count;
    size_t item_size;
    size_t capacity;
    void* items;
};

static ListImpl* Impl(List* l) { return (ListImpl*)Cast(l, TYPE_LIST); }

List* CreateList(Allocator* allocator, size_t item_size, size_t capacity)
{   
    if (capacity == 0)
	capacity = DEFAULT_CAPACITY;

    auto list = (List*)CreateObject(allocator, sizeof(ListImpl), TYPE_LIST);
    auto impl = Impl(list);
    if (!list)
        return nullptr;
    
    impl->count = 0;
    impl->capacity = capacity;
    impl->items = (void*)Alloc(allocator, item_size * capacity);
    if (!impl->items)
    {
        Destroy(list);
        return nullptr;
    }
    
    return (List*)list;
}

#if 0

// todo: destructor
#if 0
void list_free(List* list)
{
    if (!list) return;
    
    if (list->data)
        free(list->data);
    free(list);
}
#endif

size_t GetCount(List* list)
{
    return Impl(list)->count;
}

size_t GetCapacity(List* list)
{
    return Impl(list)->capacity;
}

void Add(List* list, void* value)
{
    ListImpl* impl = Impl(list);
    
    if (impl->count >= impl->capacity)
    {
        impl->capacity *= 2;
		impl->values = (void**)Realloc(GetAllocator(list), impl->values, sizeof(void*) * impl->capacity);
        if (!impl->values)
            return;
    }
    
	impl->values[impl->count++] = value;
}

void* Pop(List* list)
{
    ListImpl* impl = Impl(list);
    if (impl->count == 0)
        return nullptr;
    
    return impl->values[--impl->count];
}

void* GetAt(List* list, size_t index)
{
    ListImpl* impl = Impl(list);
	assert(index < impl->count);
	return impl->values[index];
}

void Clear(List* list)
{
    ListImpl* impl = Impl(list);
    impl->count = 0;
}

bool IsEmpty(List* list)
{
    return Impl(list)->count == 0;
}

int Find(List* list, void* value)
{
	ListImpl* impl = Impl(list);
    for (size_t i = 0; i < impl->count; i++)
        if (impl->values[i] == value)
            return (int)i;

    return -1;
}

int Find(List* list, bool (*predicate) (void*, void* data), void* data)
{
    assert(predicate);
    ListImpl* impl = Impl(list);
    for (size_t i = 0; i < impl->count; i++)
        if (predicate(impl->values[i], data))
            return (int)i;

    return -1;
}


#endif
