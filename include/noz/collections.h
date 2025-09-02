//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct List {};

// @list
List* CreateList(Allocator* allocator, size_t item_size, size_t capacity);
u32 GetCount(List* list);
void* GetAt(List* list, u32 index);
void* Add(List* list);
void* Add(List* list, const void* value);
void RemoveRange(List* list, const void* first, int count);
void Remove(List* list, const void* item);
void RemoveAt(List* list, u32 index);
void Clear(List* list);
bool IsEmpty(List* list);
bool IsFull(List* list);

// @ring_buffer
struct RingBuffer
{
    u8* items;
    u32 item_size;
    u32 capacity;
    u32 count;
    u32 front;
    u32 back;
};

RingBuffer* CreateRingBuffer(Allocator* allocator, u32 item_size, u32 capacity);
void* PushFront(RingBuffer* rb);
void* PushFront(RingBuffer* rb, const void* item);
void* PushBack(RingBuffer* rb);
void* PushBack(RingBuffer* rb, const void* item);
void PopBack(RingBuffer* rb);
void PopFront(RingBuffer* rb);
void Clear(RingBuffer* rb);
void* GetAt(RingBuffer* rb, u32 index);

inline bool IsEmpty(RingBuffer* rb) { return rb->count == 0; }
inline bool IsFull(RingBuffer* rb) { return rb->count == rb->capacity; }
inline u32 GetCount(RingBuffer* rb) { return rb->count; }

// @map
struct Map
{
    size_t capacity;
    size_t count;
    u64* keys;
    void* values;
    size_t value_stride;
};

inline void Init(Map& map, u64* keys, void* values, size_t capacity, size_t value_stride, size_t initial_count=0)
{
    map.capacity = capacity;
    map.keys = keys;
    map.values = values;
    map.value_stride = value_stride;
    map.count = initial_count;
}

bool HasKey(const Map& map, u64 key);
void* GetValue(const Map& map, const char* key);
void* GetValue(const Map& map, u64 key);
void* SetValue(Map& map, const char* key, void* value = nullptr);
void* SetValue(Map& map, u64 key, void* value = nullptr);

struct LinkedList
{
    void* head;
    void* tail;
    u16 node_offset;
    u32 count;
};

struct LinkedListNode
{
    void* next;
    void* prev;
};

// @init
void Init(LinkedList& list, u32 node_offset);
void Clear(LinkedList& list);

// @access
inline void* GetFront(LinkedList& list) { return list.head; }
inline void* GetBack(LinkedList& list) { return list.tail; }
inline bool IsEmpty(LinkedList& list) { return list.head == nullptr; }
void* GetNext(LinkedList& list, void* node);
void* GetPrev(LinkedList& list, void* node);
int GetCount(LinkedList& list);
bool IsInList(LinkedList& list, void* node);

// @insert
void PushFront(LinkedList& list, void* node);
void PushBack(LinkedList& list, void* node);
void InsertAfter(LinkedList& list, void* existing_node, void* new_node);
void InsertBefore(LinkedList& list, void* existing_node, void* new_node);

// @remove
void Remove(LinkedList& list, void* node);
void* PopFront(LinkedList& list);
void* PopBack(LinkedList& list);

// @free_list

struct FreeList
{
    u32 count;
    u32 capacity;
    void* items;
    u32 item_size;
};

void Init(FreeList& list, void* items, u32 item_size, u32 capacity);