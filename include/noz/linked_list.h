//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

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
