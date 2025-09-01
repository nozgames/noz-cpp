//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

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

#if 0
// Load VFX from binary stream
static vfx load_vfx_from_stream(binary_stream& stream)
{
    auto vfx_obj = create_object<vfx>();
    auto impl = vfx_obj.impl();

    // Read header
    impl->duration = stream.read<float>();
    impl->loop = stream.read<bool>();

    // Read emitter count
    uint32_t emitter_count = stream.read<uint32_t>();
    impl->emitters.resize(emitter_count);

    // Read each emitter
    for (uint32_t i = 0; i < emitter_count; ++i)
    {
        auto& emitter = impl->emitters[i];

        // Read emitter data
        emitter.rate = stream.read<int>();
        emitter.burst.min = stream.read<int>();
        emitter.burst.max = stream.read<int>();
        emitter.duration.min = stream.read<float>();
        emitter.duration.max = stream.read<float>();
        emitter.angle.min = stream.read<float>();
        emitter.angle.max = stream.read<float>();
        emitter.radius.min = stream.read<float>();
        emitter.radius.max = stream.read<float>();
        emitter.spawn.min = stream.read<vec3>();
        emitter.spawn.max = stream.read<vec3>();

        // Read particle data
        auto& particle = emitter.particle;
        particle.mesh_name = stream.read_string();
        particle.duration.min = stream.read<float>();
        particle.duration.max = stream.read<float>();
        particle.size.start.min = stream.read<float>();
        particle.size.start.max = stream.read<float>();
        particle.size.end.min = stream.read<float>();
        particle.size.end.max = stream.read<float>();
        particle.size.curve = static_cast<VfxCurveType>(stream.read<int>());
        particle.speed.start.min = stream.read<float>();
        particle.speed.start.max = stream.read<float>();
        particle.speed.end.min = stream.read<float>();
        particle.speed.end.max = stream.read<float>();
        particle.speed.curve = static_cast<VfxCurveType>(stream.read<int>());
        particle.Color.start.min = stream.read<Color>();
        particle.Color.start.max = stream.read<Color>();
        particle.Color.end.min = stream.read<Color>();
        particle.Color.end.max = stream.read<Color>();
        particle.Color.curve = static_cast<VfxCurveType>(stream.read<int>());
        particle.gravity = stream.read<vec3>();
        particle.drag = stream.read<float>();
        particle.rotation.min = stream.read<float>();
        particle.rotation.max = stream.read<float>();
        particle.angular_velocity.min = stream.read<float>();
        particle.angular_velocity.max = stream.read<float>();
        particle.billboard_mode = stream.read<int>();
    }

    return vfx_obj;
}

vfx load_vfx(const string& name)
{
    binary_stream stream(get_asset_path(name, "vfx"));
    auto vfx_obj = load_vfx_from_stream(stream);
    vfx_obj.impl()->name = name;
    return vfx_obj;
}

float get_duration(const vfx& vfx)
{
    auto impl = vfx.impl();
    return impl ? impl->duration : 0.0f;
}

bool is_looping(const vfx& vfx)
{
    auto impl = vfx.impl();
    return impl ? impl->loop : false;
}

#endif
