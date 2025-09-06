//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct BoneTransform
{
    Vec2 position;
    Vec2 scale;
    float rotation;
};

struct Bone
{
    const Name* name;
    i32 index;
    i32 parentIndex;
    Mat3 world_to_local;
    Mat3 local_to_world;
    BoneTransform transform;
    f32 length;
    Vec2 direction;
};

struct SkeletonImpl : Skeleton
{
    int bone_count;
    Bone* bones;
};

int GetBoneIndex(Skeleton* skeleton, const Name* name)
{
    assert(skeleton);
    assert(name);
    SkeletonImpl* impl = (SkeletonImpl*)skeleton;
    for (int i = 0; i < ((SkeletonImpl*)skeleton)->bone_count; ++i)
        if (impl->bones[i].name == name)
            return i;

    return 0;
}

const Mat3& GetLocalToWorld(Skeleton* skeleton, int bone_index)
{
    SkeletonImpl* impl = (SkeletonImpl*)skeleton;
    assert(impl);
    assert(bone_index >= 0 && bone_index < impl->bone_count);

    return impl->bones[bone_index].local_to_world;
}

const Mat3& GetWorldToLocal(Skeleton* skeleton, int bone_index)
{
    SkeletonImpl* impl = (SkeletonImpl*)skeleton;
    assert(impl);
    assert(bone_index >= 0 && bone_index < impl->bone_count);

    return impl->bones[bone_index].world_to_local;
}

Asset* LoadSkeleton(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name)
{
    SkeletonImpl* impl = (SkeletonImpl*)Alloc(allocator, sizeof(SkeletonImpl));

    impl->bone_count = ReadU8(stream);
    impl->bones = (Bone*)Alloc(allocator, sizeof(Bone) * impl->bone_count);

    for (uint32_t i = 0; i < impl->bone_count; ++i)
    {
        Bone& bone = impl->bones[i];
        bone.name = ReadName(stream);
        bone.index = ReadI8(stream);
        bone.parentIndex = ReadI8(stream);;
        bone.local_to_world= ReadStruct<Mat3>(stream);
        bone.world_to_local = ReadStruct<Mat3>(stream);
        bone.transform.position = ReadVec2(stream);
        bone.transform.rotation = ReadFloat(stream);
        bone.transform.scale = ReadVec2(stream);;
        bone.length = ReadFloat(stream);
        bone.direction = ReadVec2(stream);
    }

    return impl;
}
