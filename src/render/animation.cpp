//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

int GetBoneCount(Animation* animation)
{
    return ((AnimationImpl*)animation)->bone_count;
}

Asset* LoadAnimation(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table)
{
    AnimationImpl* impl = (AnimationImpl*)Alloc(allocator, sizeof(AnimationImpl));

    u8 bone_count = ReadU8(stream);
    u8 frame_count = ReadU8(stream);

    impl->bone_count = bone_count;
    impl->frame_count = frame_count;
    impl->bones = (AnimationBone*)Alloc(allocator, sizeof(AnimationBone) * bone_count);
    impl->frames = (BoneTransform*)Alloc(allocator, sizeof(BoneTransform) * bone_count * frame_count);
    impl->frame_stride = bone_count;
    impl->duration = (frame_count - 1) * ANIMATION_FRAME_RATE_INV;

    ReadBytes(stream, &impl->bones[0], sizeof(AnimationBone) * bone_count);
    ReadBytes(stream, &impl->frames[0], sizeof(BoneTransform) * bone_count * frame_count);

    return impl;
}
