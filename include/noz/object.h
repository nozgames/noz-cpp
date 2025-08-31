//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "allocator.h"
#include "types.h"

#define OBJECT_BASE_SIZE (sizeof(u16) * 2 + sizeof(void*) + sizeof(u32))
#define OBJECT_OFFSET_TYPE (0)
#define OBJECT_OFFSET_BASE (sizeof(u16))
#define OBJECT_OFFSET_SIZE (sizeof(u16) * 2)
#define OBJECT_OFFSET_ALLOCATOR (sizeof(u16) * 2 + sizeof(u32))

struct Object { u8 _object[OBJECT_BASE_SIZE]; };

// @object
Object* CreateObject(Allocator* allocator, size_t object_size, type_t object_type, type_t base_type);
inline Object* CreateObject(Allocator* allocator, size_t object_size, type_t object_type)
{
    return CreateObject(allocator, object_size, object_type, -1);
}

void Destroy(Object* object);

inline type_t GetType(Object* object) { return *((type_t*)((char*)object + OBJECT_OFFSET_TYPE)); }
inline type_t GetBaseType(Object* object) { return *((type_t*)((char*)object + OBJECT_OFFSET_BASE)); }
inline size_t GetAllocatedSize(Object* object) { return (size_t)(uint32_t*)((char*)object + OBJECT_OFFSET_SIZE); }
inline Allocator* GetAllocator(Object* object) { return *(Allocator**)((char*)object + OBJECT_OFFSET_ALLOCATOR); }

inline void* Cast(Object* obj, type_t type_id)
{
    assert(obj && *((type_t*)obj) == type_id);
    return obj;
}

inline void* CastToBase(Object* obj, type_t base_id)
{
    assert(obj && *(((type_t*)obj) + 1) == base_id);
    return obj;
}
