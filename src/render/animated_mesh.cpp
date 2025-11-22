//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

AnimatedMesh** ANIMATEDMESH = nullptr;

struct AnimatedMeshImpl : AnimatedMesh {
    Bounds2 bounds;
    int frame_count;
    Mesh* frames[ANIMATED_MESH_MAX_FRAMES];
};

int GetFrameCount(AnimatedMesh* mesh) {
    return static_cast<AnimatedMeshImpl*>(mesh)->frame_count;
}

Mesh* GetFrame(AnimatedMesh* mesh, int frame_index) {
    return static_cast<AnimatedMeshImpl*>(mesh)->frames[frame_index];
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
    AnimatedMeshImpl* mesh = CreateAnimatedMesh(allocator, name);
    if (!mesh)
        return nullptr;

    mesh->frame_count = frame_count;
    for (int i=0; i<frame_count; i++)
        mesh->frames[i] = frames[i];

    return mesh;
}

Asset* LoadAnimatedMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
    (void)header;
    (void)name_table;

    Bounds2 bounds = ReadStruct<Bounds2>(stream);

    AnimatedMeshImpl* impl = CreateAnimatedMesh(allocator, name);
    if (!impl)
        return nullptr;

    impl->bounds = bounds;

//    UploadMesh(impl);
    return impl;
}
