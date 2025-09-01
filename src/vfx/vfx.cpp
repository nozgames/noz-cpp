//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct VfxParticleDef
{
    VfxVec2 gravity;
    VfxFloat duration;
    VfxFloat drag;
    VfxFloatCurve size;        // Replaces start_size/end_size
    VfxFloatCurve speed;       // Replaces start_speed/end_speed
    VfxColorCurve color;       // Replaces start_color/end_color
    VfxFloatCurve rotation;
    VfxFloatCurve angular_velocity;
    //string mesh_name;
    //int billboard_mode; // 0=none, 1=camera, 2=velocity
};

struct VfxEmitterDef
{
    VfxInt   rate;
    VfxInt   burst;
    VfxFloat duration;
    VfxFloat angle;
    VfxFloat radius;
    VfxVec2  spawn;
    VfxParticleDef particle_def;
};

struct VfxImpl : Vfx
{
    VfxFloat duration;
    VfxEmitterDef* emitters;
    u32 emitter_count;
    bool loop;
};


// float GetRandom(const VfxParsedFloat& range)
// {
//     return RandomFloat(range.min, range.max);
// }
//
// int GetRandom(const VfxInt& range)
// {
//     return RandomInt(range.min, range.max);
// }
//
// Color GetRandom(const VfxRandomColor& range)
// {
//     return Lerp(range.min, range.max, RandomFloat());
// }
//
// Vec2 GetRandom(const VfxRandomVec2& range)
// {
//     return Lerp(range.min, range.max, RandomFloat());
// }

Asset* LoadVfx(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name)
{
    Vfx* vfx = (Vfx*)Alloc(allocator, sizeof(VfxImpl));
    VfxImpl* impl = static_cast<VfxImpl*>(vfx);
    impl->name = name;
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
        particle_def->angular_velocity = ReadStruct<VfxFloatCurve>(stream);
    }

    return vfx;
}
