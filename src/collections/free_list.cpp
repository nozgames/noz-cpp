//
// Created by apoxo on 9/1/2025.
//

void Init(FreeList& list, void* items, u32 item_size, u32 capacity)
{
    list.items = items;
    list.item_size = item_size;
    list.capacity = capacity;
    list.count = 0;
}


