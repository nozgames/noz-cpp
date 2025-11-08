//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

Skeleton** SKELETON = nullptr;

int GetBoneCount(Skeleton* skeleton) {
    return ((SkeletonImpl*)skeleton)->bone_count;
}

int GetBoneIndex(Skeleton* skeleton, const Name* name) {
    assert(skeleton);
    assert(name);
    SkeletonImpl* impl = (SkeletonImpl*)skeleton;
    for (int i = 0; i < ((SkeletonImpl*)skeleton)->bone_count; ++i)
        if (impl->bones[i].name == name)
            return i;

    return 0;
}

const Bone& GetBone(Skeleton* skeleton, int bone_index) {
    SkeletonImpl* impl = (SkeletonImpl*)skeleton;
    assert(impl);
    assert(bone_index >= 0 && bone_index < impl->bone_count);
    return impl->bones[bone_index];
}

const Transform& GetBoneTransform(Skeleton* skeleton, int bone_index) {
    SkeletonImpl* impl = (SkeletonImpl*)skeleton;
    assert(impl);
    assert(bone_index >= 0 && bone_index < impl->bone_count);
    return impl->bones[bone_index].transform;
}

const Mat3& GetLocalToWorld(Skeleton* skeleton, int bone_index) {
    SkeletonImpl* impl = (SkeletonImpl*)skeleton;
    assert(impl);
    assert(bone_index >= 0 && bone_index < impl->bone_count);

    return GetLocalToWorld(impl->bones[bone_index].transform);
}

const Mat3& GetWorldToLocal(Skeleton* skeleton, int bone_index) {
    SkeletonImpl* impl = (SkeletonImpl*)skeleton;
    assert(impl);
    assert(bone_index >= 0 && bone_index < impl->bone_count);

    return GetWorldToLocal(impl->bones[bone_index].transform);
}

Asset* LoadSkeleton(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
    (void)header;
    (void)name;

    SkeletonImpl* impl = (SkeletonImpl*)Alloc(allocator, sizeof(SkeletonImpl));

    impl->bone_count = ReadU8(stream);
    impl->bones = (Bone*)Alloc(allocator, sizeof(Bone) * impl->bone_count);

    for (u8 i = 0; i < impl->bone_count; ++i) {
        Bone& bone = impl->bones[i];
        bone.name = name_table[i];
        bone.index = i;
        bone.parent_index = ReadI8(stream);
        bone.transform.local_to_world = ReadStruct<Mat3>(stream);
        bone.transform.world_to_local = ReadStruct<Mat3>(stream);
        bone.transform.position = ReadStruct<Vec2>(stream);
        bone.transform.rotation = ReadFloat(stream);
        bone.transform.scale = ReadStruct<Vec2>(stream);
    }

    return impl;
}
