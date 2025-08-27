//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct List : Object {};

List* CreateList(Allocator* allocator, size_t capacity);
size_t GetCount(List* list);
size_t GetCapacity(List* list);
void Add(List* list, void* value);
void* Pop(List* list);
void* GetAt(List* list, size_t index);
void Clear(List* list);
bool IsEmpty(List* list);
int Find(List* list, void* value);
int Find(List* list, bool (*predicate) (void*, void* data), void* data);

inline void Push(List* list, void* value) { Add(list, value); }

