//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz {

    Skeleton** SKELETON = nullptr;
    int SKELETON_COUNT = 0;

    int GetBoneCount(Skeleton* skeleton) {
        return static_cast<SkeletonImpl *>(skeleton)->bone_count;
    }

    int GetBoneIndex(Skeleton* skeleton, const Name* name) {
        assert(skeleton);
        assert(name);
        SkeletonImpl* impl = static_cast<SkeletonImpl *>(skeleton);
        for (int i = 0; i < impl->bone_count; ++i)
            if (impl->bones[i].name == name)
                return i;

        return 0;
    }

    const Bone& GetBone(Skeleton* skeleton, int bone_index) {
        SkeletonImpl* impl = static_cast<SkeletonImpl *>(skeleton);
        assert(impl);
        assert(bone_index >= 0 && bone_index < impl->bone_count);
        return impl->bones[bone_index];
    }

    const Mat3& GetBindPose(Skeleton* skeleton, int bone_index) {
        SkeletonImpl* impl = static_cast<SkeletonImpl*>(skeleton);
        assert(impl);
        assert(bone_index >= 0 && bone_index < impl->bone_count);
        return impl->bones[bone_index].bind_pose;
    }

    Asset* LoadSkeleton(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
        (void)header;
        (void)name;

        SkeletonImpl* impl = static_cast<SkeletonImpl *>(Alloc(allocator, sizeof(SkeletonImpl)));

        impl->bone_count = ReadU8(stream);
        impl->bones = static_cast<Bone *>(Alloc(allocator, sizeof(Bone) * impl->bone_count));

        for (u8 i = 0; i < impl->bone_count; ++i) {
            Bone& bone = impl->bones[i];
            bone.name = name_table[i];
            bone.index = i;
            bone.parent_index = ReadI8(stream);
            bone.transform.position = ReadStruct<Vec2>(stream);
            bone.transform.rotation = ReadFloat(stream);
            bone.transform.scale = ReadStruct<Vec2>(stream);
            bone.bind_pose = ReadStruct<Mat3>(stream);
        }

        return impl;
    }

    Skeleton* CreateSkeleton(Allocator* allocator, Bone* bones, int bone_count, const Name* name) {
        SkeletonImpl* impl = static_cast<SkeletonImpl *>(Alloc(allocator, sizeof(SkeletonImpl)));
        impl->name = name;
        impl->bone_count = bone_count;
        impl->bones = static_cast<Bone *>(Alloc(allocator, sizeof(Bone) * bone_count));
        memcpy(impl->bones, bones, sizeof(Bone) * bone_count);
        return impl;
    }
}
