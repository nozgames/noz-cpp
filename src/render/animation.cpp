//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

Animation** ANIMATION = nullptr;
int ANIMATION_COUNT = 0;

float GetDuration(Animation* animation) {
    return static_cast<AnimationImpl*>(animation)->duration;
}

int GetBoneCount(Animation* animation) {
    return static_cast<AnimationImpl*>(animation)->bone_count;
}

bool IsRootMotion(Animation* animation) {
    AnimationImpl* impl = static_cast<AnimationImpl*>(animation);
    return (impl->flags & ANIMATION_FLAG_ROOT_MOTION) != 0;
}

bool IsLooping(Animation* animation) {
    AnimationImpl* impl = static_cast<AnimationImpl*>(animation);
    return (impl->flags & ANIMATION_FLAG_LOOPING) != 0;
}

Asset* LoadAnimation(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
    (void)header;
    (void)name;
    (void)name_table;

    AnimationImpl* impl = static_cast<AnimationImpl*>(Alloc(allocator, sizeof(AnimationImpl)));
    impl->name = name;

    u8 bone_count = ReadU8(stream);
    u8 transform_count = ReadU8(stream);
    u8 frame_count = ReadU8(stream);
    u8 frame_rate = ReadU8(stream);
    AnimationFlags flags = ReadU8(stream);

    impl->bone_count = bone_count;
    impl->frame_count = frame_count;
    impl->transform_count = transform_count;
    impl->bones = static_cast<AnimationBone*>(Alloc(allocator, sizeof(AnimationBone) * bone_count));
    impl->transforms = static_cast<BoneTransform*>(Alloc(allocator, sizeof(BoneTransform) * bone_count * transform_count));
    impl->frames = static_cast<AnimationFrame*>(Alloc(allocator, sizeof(AnimationFrame) * (frame_count + 1)));
    impl->transform_stride = bone_count;
    impl->frame_rate = frame_rate;
    impl->frame_rate_inv = 1.0f / static_cast<float>(frame_rate);
    impl->duration = frame_count * impl->frame_rate_inv;
    impl->flags = flags;

    ReadBytes(stream, &impl->bones[0], sizeof(AnimationBone) * bone_count);
    ReadBytes(stream, &impl->transforms[0], sizeof(BoneTransform) * bone_count * transform_count);
    ReadBytes(stream, &impl->frames[0], sizeof(AnimationFrame) * (frame_count + 1));

    impl->frames[impl->frame_count] = impl->frames[impl->frame_count-1];

    return impl;
}

Animation* CreateAnimation(
    Allocator* allocator,
    Skeleton* skeleton,
    int frame_count,
    int frame_rate,
    BoneTransform* transforms,
    int transform_count,
    AnimationEvent* events,
    int event_count,
    AnimationFlags flags,
    const Name* name) {
    assert(skeleton);
    assert(transforms);
    assert(transform_count == GetBoneCount(skeleton) * frame_count);

    frame_count--;

    AnimationImpl* impl = static_cast<AnimationImpl*>(Alloc(allocator, sizeof(AnimationImpl)));
    impl->name = name;
    impl->bone_count = GetBoneCount(skeleton);
    impl->frame_count = frame_count;
    impl->transform_count = frame_count;
    impl->bones = nullptr;
    impl->frames = static_cast<AnimationFrame*>(Alloc(allocator, sizeof(AnimationFrame) * (frame_count + 1)));
    impl->transform_stride = impl->bone_count;
    impl->frame_rate = frame_rate;
    impl->frame_rate_inv = 1.0f / static_cast<float>(impl->frame_rate);
    impl->duration = frame_count * impl->frame_rate_inv;
    impl->flags = flags;
    impl->transforms = transforms;

    for (int frame_index=0; frame_index<frame_count; frame_index++) {
        AnimationFrame& f = impl->frames[frame_index];
        f.transform0 = frame_index;
        f.transform1 = frame_index + 1;
        f.fraction0 = 0.0f;
        f.fraction1 = 1.0f;
        f.event = 0;
        f.root_motion0 = 0.0f;
        f.root_motion1 = transforms[frame_index * impl->bone_count].position.x;
        transforms[frame_index * impl->bone_count].position.x = 0.0;

        if (events) {
            for (int event_index=0; event_index<event_count; event_index++) {
                if (events[event_index].frame == frame_index) {
                    f.event = events[event_index].id;
                    break;
                }
            }
        }
    }

    if (flags & ANIMATION_FLAG_LOOPING) {
        impl->frames[frame_count - 1].transform1 = 0;
    }

    return impl;
}
