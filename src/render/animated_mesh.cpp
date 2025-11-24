//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

extern Mesh* LoadMesh(Allocator* allocator, Stream* stream, const Name* name);

AnimatedMesh** ANIMATEDMESH = nullptr;

struct AnimatedMeshImpl : AnimatedMesh {
    Bounds2 bounds;
    int frame_count;
    Mesh* frames[MAX_ANIMATION_FRAMES];
    int frame_rate;
    float frame_rate_inv;
    float duration;
};

Bounds2 GetBounds(AnimatedMesh* mesh) {
    return static_cast<AnimatedMeshImpl*>(mesh)->bounds;
}

extern Vec2 GetSize(AnimatedMesh* mesh) {
    return GetSize(GetBounds(mesh));
}

float GetDuration(AnimatedMesh* mesh) {
    return static_cast<AnimatedMeshImpl*>(mesh)->duration;
}

int GetFrameCount(AnimatedMesh* mesh) {
    return static_cast<AnimatedMeshImpl*>(mesh)->frame_count;
}

Mesh* GetFrame(AnimatedMesh* mesh, int frame_index) {
    return static_cast<AnimatedMeshImpl*>(mesh)->frames[frame_index];
}

float Update(AnimatedMesh* mesh, float current_time, float speed, bool loop) {
    AnimatedMeshImpl* impl = static_cast<AnimatedMeshImpl*>(mesh);
    if (impl->frame_count == 0)
        return 0.0f;

    current_time += speed * GetFrameTime();
    if (current_time >= impl->duration - F32_EPSILON) {
        if (loop)
            current_time = fmod(current_time, impl->duration);
        else
            current_time = impl->duration - F32_EPSILON;
    }

    return current_time;
}

int GetFrameIndex(AnimatedMesh* mesh, float time) {
    return FloorToInt(time * static_cast<float>(static_cast<AnimatedMeshImpl*>(mesh)->frame_rate));
}

static void AnimatedMeshDestructor(void* p) {
    AnimatedMeshImpl* impl = static_cast<AnimatedMeshImpl*>(p);
    for (int i=0; i<impl->frame_count; i++) {
        if (!impl->frames[i]) continue;
        Free(impl->frames[i]);
        impl->frames[i] = nullptr;
    }
    impl->frame_count = 0;
}

static AnimatedMeshImpl* CreateAnimatedMesh(Allocator* allocator, const Name* name) {
    AnimatedMeshImpl* mesh = (AnimatedMeshImpl*)Alloc(allocator, sizeof(AnimatedMeshImpl), AnimatedMeshDestructor);
    if (!mesh)
        return nullptr;

    mesh->name = name ? name : NAME_NONE;
    return mesh;
}

AnimatedMesh* CreateAnimatedMesh(Allocator* allocator, const Name* name, int frame_count, Mesh** frames) {
    AnimatedMeshImpl* impl = CreateAnimatedMesh(allocator, name);
    if (!impl)
        return nullptr;

    impl->frame_count = frame_count;
    impl->frame_rate = ANIMATION_FRAME_RATE;
    impl->frame_rate_inv = 1.0f / static_cast<float>(impl->frame_rate);
    impl->duration = impl->frame_count * impl->frame_rate_inv;
    for (int i=0; i<frame_count; i++)
        impl->frames[i] = frames[i];

    return impl;
}

Asset* LoadAnimatedMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
    (void)header;
    (void)name_table;

    AnimatedMeshImpl* impl = CreateAnimatedMesh(allocator, name);
    if (!impl)
        return nullptr;

    impl->bounds = ReadStruct<Bounds2>(stream);
    impl->frame_rate = ReadU8(stream);
    impl->frame_rate_inv = 1.0f / static_cast<float>(impl->frame_rate);
    impl->frame_count = ReadU8(stream);
    impl->duration = impl->frame_count * impl->frame_rate_inv;

    for (int frame_index=0; frame_index<impl->frame_count; frame_index++)
        impl->frames[frame_index] = LoadMesh(allocator, stream, NAME_NONE);

    return impl;
}
