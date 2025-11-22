//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

AnimatedMesh** ANIMATED_MESH = nullptr;

struct AnimatedMeshImpl : Mesh {
    Bounds2 bounds;
};

static void AnimatedMeshDestructor(void* p) {
    AnimatedMeshImpl* impl = static_cast<AnimatedMeshImpl*>(p);
    (void)impl;
}

static AnimatedMeshImpl* CreateAnimatedMesh(Allocator* allocator, const Name* name) {
    AnimatedMeshImpl* mesh = (AnimatedMeshImpl*)Alloc(allocator, sizeof(AnimatedMeshImpl), AnimatedMeshDestructor);
    if (!mesh)
        return nullptr;

    mesh->name = name ? name : NAME_NONE;
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