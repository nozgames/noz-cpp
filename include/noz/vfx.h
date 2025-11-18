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
VfxHandle Play(Vfx* vfx, const Vec2& position);
VfxHandle Play(Vfx* vfx, const Mat3& transform);
void Stop(const VfxHandle& handle);
bool IsPlaying(const VfxHandle& handle);
void ClearVfx();
void DrawVfx();
Bounds2 GetBounds(Vfx* vfx);

constexpr VfxHandle INVALID_VFX_HANDLE = { 0xFFFFFFFF, 0xFFFFFFFF };

extern Vfx** VFX;