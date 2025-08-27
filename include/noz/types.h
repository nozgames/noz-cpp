//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef u16 type_t;

enum Type : type_t
{
    TYPE_INVALID = (u16)-1,
    TYPE_UNKNOWN = (u16)-1000,
    TYPE_STREAM,
    TYPE_LIST,
    TYPE_MAP,
    TYPE_PROPS,
    TYPE_MESH_BUILDER,
    TYPE_INPUT_SET,
    TYPE_COLLIDER,
    TYPE_RIGID_BODY,
    TYPE_MATERIAL,
    TYPE_SHADER,
    TYPE_FONT,
    TYPE_MESH,
    TYPE_SOUND,
    TYPE_TEXTURE,
    TYPE_STYLE_SHEET,
    TYPE_ENTITY,
    TYPE_CAMERA,
    TYPE_MESH_RENDERER,
    TYPE_CANVAS,
    TYPE_ELEMENT,
    TYPE_TEXT_MESH,
    TYPE_LABEL,
    TYPE_SCENE,
};

constexpr int TYPE_COUNT = 0xFFFF + 1;

struct Object;

struct TypeTraits
{
    void (*destructor)(Object* object);
};

// @type
extern const TypeTraits* g_type_traits[];

void SetTypeTraits(type_t id, const TypeTraits* traits);

inline const TypeTraits* GetTypeTraits(type_t id)
{
    return g_type_traits[id];
}