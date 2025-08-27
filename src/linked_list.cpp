//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define GET_NODE(list, node) ((LinkedListNode*)((char*)(node) + (list).node_offset))
#define GET_NEXT(list, node) (GET_NODE(list, node)->next)
#define GET_PREV(list, node) (GET_NODE(list, node)->prev)
#define SET_NEXT(list, node, value) (GET_NODE(list, node)->next = (value))
#define SET_PREV(list, node, value) (GET_NODE(list, node)->prev = (value))

// @init
void Init(LinkedList& list, u32 node_offset)
{
    list.head = nullptr;
    list.tail = nullptr;
    list.node_offset = node_offset;
    list.count = 0;
}

int GetCount(LinkedList& list)
{
    return list.count;
}

void Clear(LinkedList& list)
{
    // Walk through the list and clear all node pointers to prevent dangling references
    void* current = list.head;
    while (current)
    {
        void* next = GET_NEXT(list, current);
        SET_NEXT(list, current, nullptr);
        SET_PREV(list, current, nullptr);
        current = next;
    }
    
    list.head = nullptr;
    list.tail = nullptr;
    list.count = 0;
}

// @insert
void PushFront(LinkedList& list, void* node)
{
    assert(node);

    SET_NEXT(list, node, list.head);
    SET_PREV(list, node, nullptr);
    
    if (list.head)
        SET_PREV(list, list.head, node);
    else
        list.tail = node;
        
    list.head = node;
    list.count++;
}

void PushBack(LinkedList& list, void* node)
{
    assert(node);

    SET_NEXT(list, node, nullptr);
    SET_PREV(list, node, list.tail);
    
    if (list.tail)
        SET_NEXT(list, list.tail, node);
    else
        list.head = node;
        
    list.tail = node;
    list.count++;
}

void InsertAfter(LinkedList& list, void* existing_node, void* new_node)
{
    assert(existing_node);
    assert(new_node);

    void* next = GET_NEXT(list, existing_node);
    
    SET_NEXT(list, new_node, next);
    SET_PREV(list, new_node, existing_node);
    SET_NEXT(list, existing_node, new_node);
    
    if (next)
        SET_PREV(list, next, new_node);
    else
        list.tail = new_node;

    list.count++;
}

void InsertBefore(LinkedList& list, void* existing_node, void* new_node)
{
    assert(existing_node);
    assert(new_node);

    void* prev = GET_PREV(list, existing_node);
    
    SET_NEXT(list, new_node, existing_node);
    SET_PREV(list, new_node, prev);
    SET_PREV(list, existing_node, new_node);
    
    if (prev)
        SET_NEXT(list, prev, new_node);
    else
        list.head = new_node;

    list.count++;
}

// @remove
void Remove(LinkedList& list, void* node)
{
    assert(node);
        
    void* next = GET_NEXT(list, node);
    void* prev = GET_PREV(list, node);
    
    if (prev)
        SET_NEXT(list, prev, next);
    else
        list.head = next;
        
    if (next)
        SET_PREV(list, next, prev);
    else
        list.tail = prev;
        
    // Clear the node's pointers
    SET_NEXT(list, node, nullptr);
    SET_PREV(list, node, nullptr);

    list.count--;
}

void* PopFront(LinkedList& list)
{
    if (!list.head)
        return nullptr;
        
    void* node = list.head;
    Remove(list, node);
    return node;
}

void* PopBack(LinkedList& list)
{
    if (!list.tail)
        return nullptr;
        
    void* node = list.tail;
    Remove(list, node);
    return node;
}

void* GetNext(LinkedList& list, void* node)
{
    assert(node);
    return GET_NEXT(list, node);
}

void* GetPrev(LinkedList& list, void* node)
{
    assert(node);
    return GET_PREV(list, node);
}

bool IsInList(LinkedList& list, void* node)
{
    assert(node);
    return GetFront(list) == node || GetPrev(list, node) != nullptr;
}
