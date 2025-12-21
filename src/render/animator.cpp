//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

constexpr float ANIMATOR_BLEND_TIME = 0.05f;

void AddEvent(Animator& animator, int event_id) {
    if (event_id <= 0) return;
    if (animator.event_count >= ANIMATION_MAX_EVENTS) {
        for (int i=1; i<ANIMATION_MAX_EVENTS; i++)
            animator.events[i-1] = animator.events[i];
        animator.events[animator.event_count] = event_id;
    } else {
        animator.events[animator.event_count++] = event_id;
    }
}

extern int GetEvent(Animator& animator) {
    if (animator.event_count==0) return 0;
    int event_id = animator.events[0];
    for (int i=1; i<animator.event_count; i++)
        animator.events[i-1] = animator.events[i];
    animator.event_count--;
    return event_id;
}

static void EvalulateFrame(Animator& animator, int layer_index) {
    assert(animator.skeleton);

    AnimatorLayer& layer = GetLayer(animator, layer_index);
    Animation* animation = layer.animation;
    if (!animation)
        return;

    AnimationImpl* anim_impl = static_cast<AnimationImpl*>(animation);
    f32 frame_index_float = layer.time * anim_impl->frame_rate;
    i32 frame_index = static_cast<i32>(frame_index_float);
    AnimationFrame& frame = anim_impl->frames[frame_index];
    i32 transform_index0 = frame.transform0;
    i32 transform_index1 = frame.transform1;
    f32 frame_fraction = (frame_index_float - static_cast<f32>(frame_index)) * (frame.fraction1 - frame.fraction0) + frame.fraction0;
    BoneTransform* transform0 = anim_impl->transforms + transform_index0 * anim_impl->transform_stride;
    BoneTransform* transform1 = anim_impl->transforms + transform_index1 * anim_impl->transform_stride;

    // Blend
    BoneTransform* blend_transform0 = nullptr;
    BoneTransform* blend_transform1 = nullptr;
    f32 blend_frame_fraction = 0.0f;
    f32 blend_t = 0.0f;
    if (layer.blend_animation) {
        AnimationImpl* blend_anim_impl = static_cast<AnimationImpl*>(layer.blend_animation);
        assert(blend_anim_impl);
        assert(GetBoneCount(animator.skeleton) == blend_anim_impl->bone_count);
        f32 blend_index_float = layer.blend_frame_time * blend_anim_impl->frame_rate;
        i32 blend_index = static_cast<i32>(blend_index_float);
        AnimationFrame& blend_frame = blend_anim_impl->frames[blend_index];
        i32 blend_transform_index0 = blend_frame.transform0;
        i32 blend_transform_index1 = blend_frame.transform1;
        blend_frame_fraction = (blend_index_float - static_cast<f32>(blend_index)) * (blend_frame.fraction1 - blend_frame.fraction0) + blend_frame.fraction0;
        blend_transform0 = blend_anim_impl->transforms + blend_transform_index0 * blend_anim_impl->transform_stride;
        blend_transform1 = blend_anim_impl->transforms + blend_transform_index1 * blend_anim_impl->transform_stride;
        blend_t = layer.blend_time / ANIMATOR_BLEND_TIME;
    }

    for (int bone_index=0; bone_index<anim_impl->bone_count; bone_index++) {
        if ((layer.bone_mask & (static_cast<u64>(1) << static_cast<u64>(bone_index))) == 0)
            continue;

        BoneTransform* bone_transform0 = transform0 + bone_index;
        BoneTransform* bone_transform1 = transform1 + bone_index;
        BoneTransform frame_transform = Mix(*bone_transform0, *bone_transform1, frame_fraction);

        if (blend_transform0) {
            assert(blend_transform1);
            BoneTransform* bbt1 = blend_transform0 + bone_index;
            BoneTransform* bbt2 = blend_transform1 + bone_index;
            BoneTransform blend_frame = Mix(*bbt1, *bbt2, blend_frame_fraction);
            frame_transform = Mix(blend_frame, frame_transform, blend_t);
        }

        animator.transforms[bone_index] = frame_transform;
    }

    // Root motion
    float rm = frame.root_motion0 + (frame.root_motion1 - frame.root_motion0) * frame_fraction;
    animator.root_motion_delta = rm - animator.last_root_motion;
    animator.last_root_motion = rm;

    // Events
    if (frame_index > layer.frame_index) {
        for (int i=layer.frame_index+1; i<= frame_index; i++)
            AddEvent(animator, anim_impl->frames[i].event);
    } else if (frame_index < layer.frame_index) {
        for (int i=layer.frame_index+1; i<anim_impl->frame_count; i++)
            AddEvent(animator, anim_impl->frames[i].event);
        for (int i=0; i<= frame_index; i++)
            AddEvent(animator, anim_impl->frames[i].event);
    }

    layer.frame_index = frame_index;
}

static void EvalulateFrame(Animator& animator) {
    for (int layer_index=0; layer_index<animator.layer_count; layer_index++)
        EvalulateFrame(animator, layer_index);

    SkeletonImpl* skel_impl = static_cast<SkeletonImpl*>(animator.skeleton);
    int bone_count = skel_impl->bone_count;
    animator.bones[0] = ToMat3(animator.transforms[0]);
    for (int bone_index=1; bone_index<bone_count; bone_index++) {
        int parent_index = skel_impl->bones[bone_index].parent_index;
        animator.bones[bone_index] = animator.bones[parent_index] * ToMat3(animator.transforms[bone_index]);
    }
}

void Stop(Animator& animator, int layer_index) {
    AnimatorLayer& layer = GetLayer(animator, layer_index);
    layer.animation = nullptr;
    layer.blend_animation = nullptr;
    layer.playing = false;
}

void Stop(Animator& animator) {
    for (int layer_index=0; layer_index<animator.layer_count; layer_index++) {
        Stop(animator, layer_index);
    }
}

void Play(Animator& animator, Animation* animation, int layer_index, float speed, float normalized_time, bool loop) {
    assert(animator.skeleton);
    assert(GetBoneCount(animator.skeleton) == GetBoneCount(animation));

    AnimatorLayer& layer = GetLayer(animator, layer_index);
    if (animation == layer.animation)
        return;

    layer.blend_animation = layer.animation;
    layer.blend_time = 0.0f;
    layer.blend_frame_time = layer.time;
    layer.blend_loop = layer.loop;
    layer.animation = animation;
    layer.speed = speed;
    layer.time = normalized_time * static_cast<AnimationImpl*>(animation)->duration;
    layer.loop = loop && IsLooping(layer.animation);
    layer.playing = true;
    layer.frame_index = -1;

    if ((layer.bone_mask & 0x1)) {
        animator.last_root_motion = 0.0f;
        animator.root_motion_delta = 0.0f;
        animator.root_motion = IsRootMotion(animation);
    }

    EvalulateFrame(animator);
}

void Update(Animator& animator, float time_scale) {
    float dt = GetFrameTime() * time_scale;

    for (int layer_index=0; layer_index<animator.layer_count; layer_index++) {
        AnimatorLayer& layer = GetLayer(animator, layer_index);
        if (!layer.playing || !layer.animation)
            continue;

        AnimationImpl* anim_impl = static_cast<AnimationImpl*>(layer.animation);
        layer.time += dt * layer.speed;

        if (layer.time >= anim_impl->duration) {
            if (layer.loop) {
                layer.time = fmod(layer.time, anim_impl->duration);
                animator.last_root_motion -= anim_impl->frames[anim_impl->frame_count-1].root_motion1;
            } else {
                layer.playing = false;
                layer.time = anim_impl->duration;
            }
        }

        assert(layer.time <= anim_impl->duration);

        if (layer.blend_animation) {
            layer.blend_time += dt;
            if (layer.blend_time >= ANIMATOR_BLEND_TIME) {
                layer.blend_animation = nullptr;
                layer.blend_time = 0.0f;
                layer.blend_frame_time = 0.0f;
            }
        }
    }

    EvalulateFrame(animator);
}

float GetNormalizedTime(Animator& animator, int layer_index) {
    AnimatorLayer& layer = animator.layers[layer_index];
    if (!layer.animation)
        return 0.0f;

    return layer.time / static_cast<AnimationImpl*>(layer.animation)->duration;
}

void SetNormalizedTime(Animator& animator, int layer_index, float normalized_time) {
    AnimatorLayer& layer = animator.layers[layer_index];
    if (layer.animation == nullptr)
        return;

    layer.time = normalized_time * static_cast<AnimationImpl*>(layer.animation)->duration;
    layer.time = Clamp(layer.time, 0.0f, static_cast<AnimationImpl*>(layer.animation)->duration);

    Update(animator, 0.0f);

    if (layer_index == 0)
        animator.root_motion_delta = 0.0f;
}

void SetBoneMask(Animator& animator, int layer_index, u64 bone_mask) {
    AnimatorLayer& layer = GetLayer(animator, layer_index);
    layer.bone_mask = bone_mask;
}


void BindSkeleton(Skeleton* skeleton) {
    SkeletonImpl* skel_impl = static_cast<SkeletonImpl*>(skeleton);
    Mat3 identity = MAT3_IDENTITY;
    BindSkeleton(&skel_impl->bones->bind_pose, sizeof(Bone), &identity, 0, skel_impl->bone_count);
}

void BindSkeleton(Animator& animator) {
    SkeletonImpl* skel_impl = static_cast<SkeletonImpl*>(animator.skeleton);
    int bone_count = skel_impl->bone_count;
    BindSkeleton(&skel_impl->bones->bind_pose, sizeof(Bone), animator.bones, sizeof(Mat3), bone_count);
}

void Init(Animator& animator, Skeleton* skeleton, int layer_count) {
    animator = {};
    animator.skeleton = skeleton;
    animator.layer_count = layer_count;

    for (int layer_index=0; layer_index<layer_count; layer_index++) {
        AnimatorLayer& layer = animator.layers[layer_index];
        layer.speed = 1.0f;
        layer.bone_mask = 0xFFFFFFFFFFFFFFFF;
    }

    int bone_count = GetBoneCount(skeleton);
    for (int bone_index=0; bone_index<bone_count; bone_index++) {
        animator.bones[bone_index] = MAT3_IDENTITY;
        animator.transforms[bone_index] = {
            VEC2_ZERO,
            VEC2_ONE,
            0.0f,
        };
    }
}
