//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "asset.h"

struct Vfx;

enum VfxCurveType
{
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

struct VfxFloat
{
    float min;
    float max;
};

struct VfxInt
{
    int min;
    int max;
};

struct VfxColor
{
    Color min;
    Color max;
};

struct VfxVec2
{
    Vec2 min;
    Vec2 max;
};

struct VfxFloatCurve
{
    VfxCurveType type;
    VfxFloat start;
    VfxFloat end;
};

struct VfxColorCurve
{
    VfxCurveType type;
    VfxColor start;
    VfxColor end;
};

struct Vfx : Asset { };

constexpr VfxInt VFX_INT_ZERO = { 0, 0 };
constexpr VfxFloat VFX_FLOAT_ZERO = { 0, 0 };
constexpr VfxFloat VFX_FLOAT_ONE = { 1, 1 };
constexpr VfxFloatCurve VFX_FLOAT_CURVE_ZERO = { VFX_CURVE_TYPE_LINEAR, { 0.0f, 0.0f}, {0.0f, 0.0f} };
constexpr VfxFloatCurve VFX_FLOAT_CURVE_ONE = { VFX_CURVE_TYPE_LINEAR, { 1.0f, 1.0f}, {1.0f, 1.0f} };
constexpr VfxColorCurve VFX_COLOR_CURVE_WHITE = { VFX_CURVE_TYPE_LINEAR, { {1,1,1,1}, {1,1,1,1}}, {{1,1,1,1}, {1,1,1,1}} };
constexpr VfxVec2 VFX_VEC2_ZERO = { { 0.0f, 0.0f }, {0.0f, 0.0f} };

// @vfx


// Random value generation
// float GetRandom(const VfxParsedFloat& range);
// int GetRandom(const VfxInt& range);
// Color GetRandom(const VfxRandomColor& range);
// Vec2 GetRandom(const VfxRandomVec2& range);

#if 0
// VFX resource functions
vfx load_vfx(const string& name);
float get_duration(const vfx& vfx);
bool is_looping(const vfx& vfx);

// VFX renderer functions
vfx_renderer create_vfx_renderer();
vfx_renderer create_vfx_renderer(const entity& parent);
void set_vfx(const vfx_renderer& renderer, const vfx& vfx);
void set_max_particles(const vfx_renderer& renderer, int max_particles);
void set_playback_speed(const vfx_renderer& renderer, float speed);

// Playback control
void play(const vfx_renderer& renderer);
void play(const vfx_renderer& renderer, const vec3& position);
void stop(const vfx_renderer& renderer);
void stop(const vfx_renderer& renderer, bool clear_particles);
bool is_playing(const vfx_renderer& renderer);

#endif