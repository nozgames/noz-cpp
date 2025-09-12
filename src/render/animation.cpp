//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct AnimationBone
{

};

struct AnimationImpl : Animation
{
    int bone_count;
    Mat3* frames;
};

Asset* LoadAnimation(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table)
{
    AnimationImpl* impl = (AnimationImpl*)Alloc(allocator, sizeof(AnimationImpl));

    // impl->bone_count = ReadU8(stream);
    // impl->bones = (Bone*)Alloc(allocator, sizeof(Bone) * impl->bone_count);

    // for (uint32_t i = 0; i < impl->bone_count; ++i)
    // {
    //     Bone& bone = impl->bones[i];
    //     bone.name = ReadName(stream);
    //     bone.index = ReadI8(stream);
    //     bone.parentIndex = ReadI8(stream);;
    //     bone.local_to_world= ReadStruct<Mat3>(stream);
    //     bone.world_to_local = ReadStruct<Mat3>(stream);
    //     bone.transform.position = ReadVec2(stream);
    //     bone.transform.rotation = ReadFloat(stream);
    //     bone.transform.scale = ReadVec2(stream);;
    //     bone.length = ReadFloat(stream);
    //     bone.direction = ReadVec2(stream);
    // }

    return impl;
}
