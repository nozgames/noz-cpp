//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "vfx_internal.h"

namespace noz {

    Vfx** VFX = nullptr;
    int VFX_COUNT = 0;

    static void LoadVfxInternal(VfxImpl* impl, Allocator* allocator, Stream* stream) {
        impl->bounds = ReadStruct<Bounds2>(stream);
        impl->duration = ReadStruct<VfxFloat>(stream);
        impl->loop = ReadBool(stream);
        impl->emitter_count = ReadU32(stream);

        if (impl->emitter_count > 0) {
            impl->emitters = (VfxEmitterDef*)Alloc(allocator, sizeof(VfxEmitterDef) * impl->emitter_count);

            for (u32 i = 0; i < impl->emitter_count; ++i) {
                VfxEmitterDef* emitter_def = &impl->emitters[i];
                emitter_def->rate = ReadStruct<VfxInt>(stream);
                emitter_def->burst = ReadStruct<VfxInt>(stream);
                emitter_def->duration = ReadStruct<VfxFloat>(stream);
                emitter_def->angle = ReadStruct<VfxFloat>(stream);
                emitter_def->spawn = ReadStruct<VfxVec2>(stream);
                emitter_def->direction = ReadStruct<VfxVec2>(stream);

                VfxParticleDef* particle_def = &emitter_def->particle_def;
                particle_def->duration = ReadStruct<VfxFloat>(stream);
                particle_def->size = ReadStruct<VfxFloatCurve>(stream);
                particle_def->speed = ReadStruct<VfxFloatCurve>(stream);
                particle_def->color = ReadStruct<VfxColorCurve>(stream);
                particle_def->opacity = ReadStruct<VfxFloatCurve>(stream);
                particle_def->gravity = ReadStruct<VfxVec2>(stream);
                particle_def->drag = ReadStruct<VfxFloat>(stream);
                particle_def->rotation = ReadStruct<VfxFloatCurve>(stream);
                particle_def->mesh_name = ReadName(stream);
                particle_def->mesh = GetMesh(particle_def->mesh_name);

                emitter_def->vfx = impl;
            }
        }
    }

    Bounds2 GetBounds(Vfx* vfx) {
        return static_cast<VfxImpl*>(vfx)->bounds;
    }

    Asset* LoadVfx(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
        (void)header;
        (void)name_table;

        Vfx* vfx = (Vfx*)Alloc(allocator, sizeof(VfxImpl));
        VfxImpl* impl = static_cast<VfxImpl*>(vfx);
        impl->name = name;
        LoadVfxInternal(impl, allocator, stream);
        return vfx;
    }

#if !defined(NOZ_BUILTIN_ASSETS)

    void RestartVfx(Vfx* vfx);

    void ReloadVfx(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table) {
        (void)header;
        (void)name_table;

        assert(asset);
        assert(stream);

        VfxImpl* impl = static_cast<VfxImpl*>(asset);

        VfxEmitterDef* old_emitters = impl->emitters;

        LoadVfxInternal(impl, ALLOCATOR_DEFAULT, stream);

        RestartVfx(impl);

        // We can free the emitters now that we restarted the vfx
        if (old_emitters)
            Free(old_emitters);
    }

#endif
}
