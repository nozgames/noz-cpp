//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef u16 type_t;

constexpr type_t TYPE_INVALID = -1;
constexpr type_t TYPE_UNKNOWN = -1000;
constexpr int TYPE_COUNT = 0xFFFF + 1;

// @object
constexpr type_t TYPE_STREAM = -900;
constexpr type_t TYPE_LIST = -901;
constexpr type_t TYPE_MAP = -902;
constexpr type_t TYPE_PROPS = -903;
constexpr type_t TYPE_MESH_BUILDER = -904;
constexpr type_t TYPE_INPUT_SET = -905;
constexpr type_t TYPE_COLLIDER = -906;
constexpr type_t TYPE_RIGID_BODY = -907;

// @asset
constexpr type_t TYPE_MATERIAL = -800;
constexpr type_t TYPE_SHADER = -801;
constexpr type_t TYPE_FONT = -802;
constexpr type_t TYPE_MESH = -803;
constexpr type_t TYPE_SOUND = -804;
constexpr type_t TYPE_TEXTURE = -805;
constexpr type_t TYPE_STYLE_SHEET = -806;

// @scene
constexpr type_t TYPE_ENTITY = -700;
constexpr type_t TYPE_CAMERA = -701;
constexpr type_t TYPE_MESH_RENDERER = -702;

// @ui
constexpr type_t TYPE_CANVAS = -600;
constexpr type_t TYPE_ELEMENT = -601;
constexpr type_t TYPE_TEXT_MESH = -602;
constexpr type_t TYPE_LABEL = -603;

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