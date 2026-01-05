//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

enum VfxCurveType {
    VFX_CURVE_TYPE_UNKNOWN = -1,
    VFX_CURVE_TYPE_LINEAR = 0,
    VFX_CURVE_TYPE_EASE_IN,
    VFX_CURVE_TYPE_EASE_OUT,
    VFX_CURVE_TYPE_EASE_IN_OUT,
    VFX_CURVE_TYPE_QUADRATIC,
    VFX_CURVE_TYPE_CUBIC,
    VFX_CURVE_TYPE_SINE,
    VFX_CURVE_TYPE_CUSTOM
};

struct VfxFloat {
    float min;
    float max;
};

struct VfxInt {
    int min;
    int max;
};

struct VfxColor {
    Color min;
    Color max;
};

struct VfxVec2 {
    Vec2 min;
    Vec2 max;
};

struct VfxFloatCurve {
    VfxCurveType type;
    VfxFloat start;
    VfxFloat end;
};

struct VfxColorCurve {
    VfxCurveType type;
    VfxColor start;
    VfxColor end;
};

struct VfxParticleDef {
    VfxVec2 gravity;
    VfxFloat duration;
    VfxFloat drag;
    VfxFloatCurve size;
    VfxFloatCurve speed;
    VfxColorCurve color;
    VfxFloatCurve opacity;
    VfxFloatCurve rotation;
    Mesh* mesh;
    const Name* mesh_name;
};

struct VfxEmitterDef {
    VfxInt   rate;
    VfxInt   burst;
    VfxFloat duration;
    VfxFloat angle;
    VfxVec2  spawn;
    VfxVec2  direction;
    VfxParticleDef particle_def;
    Vfx* vfx;
};

struct VfxImpl : Vfx {
    VfxFloat duration;
    VfxEmitterDef* emitters;
    u32 emitter_count;
    Bounds2 bounds;
    bool loop;
};

constexpr VfxInt VFX_INT_ZERO = { 0, 0 };
constexpr VfxFloat VFX_FLOAT_ZERO = { 0, 0 };
constexpr VfxFloat VFX_FLOAT_ONE = { 1, 1 };
constexpr VfxFloatCurve VFX_FLOAT_CURVE_ZERO = { VFX_CURVE_TYPE_LINEAR, { 0.0f, 0.0f}, {0.0f, 0.0f} };
constexpr VfxFloatCurve VFX_FLOAT_CURVE_ONE = { VFX_CURVE_TYPE_LINEAR, { 1.0f, 1.0f}, {1.0f, 1.0f} };
constexpr VfxColorCurve VFX_COLOR_CURVE_WHITE = { VFX_CURVE_TYPE_LINEAR, { {1,1,1,1}, {1,1,1,1}}, {{1,1,1,1}, {1,1,1,1}} };
constexpr VfxVec2 VFX_VEC2_ZERO = { { 0.0f, 0.0f }, {0.0f, 0.0f} };
