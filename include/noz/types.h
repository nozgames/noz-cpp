//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef u16 type_t;

#define NOZ_CORE_TYPES\
    NOZ_TYPE(STREAM) \
    NOZ_TYPE(LIST) \
    NOZ_TYPE(MAP) \
    NOZ_TYPE(PROPS) \
    NOZ_TYPE(MESH_BUILDER) \
    NOZ_TYPE(INPUT_SET) \
    NOZ_TYPE(COLLIDER) \
    NOZ_TYPE(RIGID_BODY) \
    NOZ_TYPE(MATERIAL) \
    NOZ_TYPE(SHADER) \
    NOZ_TYPE(FONT) \
    NOZ_TYPE(MESH) \
    NOZ_TYPE(SOUND) \
    NOZ_TYPE(TEXTURE) \
    NOZ_TYPE(STYLE_SHEET) \
    NOZ_TYPE(ENTITY) \
    NOZ_TYPE(COMPONENT) \
    NOZ_TYPE(CAMERA) \
    NOZ_TYPE(MESH_RENDERER) \
    NOZ_TYPE(CANVAS) \
    NOZ_TYPE(ELEMENT) \
    NOZ_TYPE(TEXT_MESH) \
    NOZ_TYPE(LABEL)

enum Type : type_t
{
#define NOZ_TYPE(type) TYPE_##type,
    TYPE_INVALID = (u16)0,
    TYPE_GAME = (u16)1,
    TYPE_UNKNOWN = (u16)256,
    NOZ_CORE_TYPES
#undef NOZ_TYPE
};

constexpr int TYPE_COUNT = 512;

struct Object;

struct TypeTraits
{
    void (*destructor)(Object* object);
};

// @type
extern const TypeTraits* g_type_traits[];

void SetTypeTraits(type_t id, const TypeTraits* traits);

extern const char* g_type_names[];

inline const char* GetTypeName(type_t id)
{
    return g_type_names[id];
}

void SetTypeName(type_t type, const char* name);

inline const TypeTraits* GetTypeTraits(type_t id)
{
    return g_type_traits[id];
}