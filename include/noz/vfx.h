//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz {

    struct Vfx : Asset { };

    struct VfxHandle {
        u32 id;
        u32 version;
    };

    // @vfx
    VfxHandle Play(Vfx* vfx, const Vec2& position, float depth=0.0f);
    VfxHandle Play(Vfx* vfx, const Mat3& transform, float depth=0.0f);
    void Stop(const VfxHandle& handle);
    bool IsPlaying(const VfxHandle& handle);
    void ClearVfx();
    void DrawVfx();
    Bounds2 GetBounds(Vfx* vfx);

    constexpr VfxHandle INVALID_VFX_HANDLE = { 0xFFFFFFFF, 0xFFFFFFFF };

    extern Vfx** VFX;
    extern int VFX_COUNT;
}
