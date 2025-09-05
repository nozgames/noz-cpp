//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "vfx_internal.h"

static void LoadVfxInternal(VfxImpl* impl, Allocator* allocator, Stream* stream)
{
    impl->duration = ReadStruct<VfxFloat>(stream);
    impl->loop = ReadBool(stream);
    impl->emitter_count = ReadU32(stream);
    impl->emitters = (VfxEmitterDef*)Alloc(allocator, sizeof(VfxEmitterDef) * impl->emitter_count);

    for (u32 i = 0; i < impl->emitter_count; ++i)
    {
        VfxEmitterDef* emitter_def = &impl->emitters[i];
        emitter_def->rate = ReadStruct<VfxInt>(stream);
        emitter_def->burst = ReadStruct<VfxInt>(stream);
        emitter_def->duration = ReadStruct<VfxFloat>(stream);
        emitter_def->angle = ReadStruct<VfxFloat>(stream);
        emitter_def->radius = ReadStruct<VfxFloat>(stream);
        emitter_def->spawn = ReadStruct<VfxVec2>(stream);

        VfxParticleDef* particle_def = &emitter_def->particle_def;
        char mesh_name[1024];
        ReadString(stream, mesh_name, 1024);

        particle_def->duration = ReadStruct<VfxFloat>(stream);
        particle_def->size = ReadStruct<VfxFloatCurve>(stream);
        particle_def->speed = ReadStruct<VfxFloatCurve>(stream);
        particle_def->color = ReadStruct<VfxColorCurve>(stream);
        particle_def->gravity = ReadStruct<VfxVec2>(stream);
        particle_def->drag = ReadStruct<VfxFloat>(stream);
        particle_def->rotation = ReadStruct<VfxFloatCurve>(stream);

        emitter_def->vfx = impl;
    }

}

Asset* LoadVfx(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name)
{
    Vfx* vfx = (Vfx*)Alloc(allocator, sizeof(VfxImpl));
    VfxImpl* impl = static_cast<VfxImpl*>(vfx);
    impl->name = name;
    LoadVfxInternal(impl, allocator, stream);
    return vfx;
}

#ifdef NOZ_EDITOR

void RestartVfx(Vfx* vfx);

void ReloadVfx(Asset* asset, Stream* stream)
{
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