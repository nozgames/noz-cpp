//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/list.h>

#define DEFAULT_CAPACITY 32

struct ListImpl
{
    OBJECT_BASE;
    void** values;
    size_t count;
    size_t capacity;
};

static ListImpl* Impl(List* l) { return (ListImpl*)Cast(l, TYPE_LIST); }

List* CreateList(Allocator* allocator, size_t capacity)
{   
    if (capacity == 0)
		capacity = DEFAULT_CAPACITY;
    
    ListImpl* list = Impl((List*)CreateObject(allocator, sizeof(ListImpl), TYPE_LIST));
    if (!list)
        return nullptr;
    
    list->count = 0;
    list->capacity = capacity;
    list->values = (void**)Alloc(allocator, sizeof(void*) * capacity);
    if (!list->values)
    {
        Destroy((List*)list);
        return nullptr;
    }
    
    return (List*)list;
}

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
