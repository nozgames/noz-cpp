//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "asset.h"

struct Vfx;

struct Vfx : Asset { };

struct VfxHandle
{
    u32 id;
    u32 version;
};

// @vfx
VfxHandle Play(Vfx* vfx, const Mat3& transform);
void Stop(const VfxHandle& handle);
bool IsPlaying(const VfxHandle& handle);

constexpr VfxHandle INVALID_HANDLE = { 0xFFFFFFFF, 0xFFFFFFFF };

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