//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

constexpr float ANIMATOR_BLEND_TIME = 0.05f;

static void EvalulateFrame(Animator& animator, int layer_index, bool setup) {
    Skeleton* skeleton = animator.skeleton;
    assert(animator.skeleton);

    AnimatorLayer& layer = GetLayer(animator, layer_index);
    Animation* animation = layer.animation;
    if (!animation)
        return;

    AnimationImpl* anim_impl = static_cast<AnimationImpl*>(animation);
    SkeletonImpl* skel_impl = static_cast<SkeletonImpl*>(skeleton);

    f32 float_frame = layer.time * anim_impl->frame_rate;
    i32 frame_index1 = static_cast<i32>(Floor(float_frame));
    f32 t = float_frame - static_cast<f32>(frame_index1);
    frame_index1 = Min(frame_index1, anim_impl->frame_count - 1);
    i32 frame_index2 = frame_index1 + 1;
    assert(t >= 0.0f && t < 1.0f);
    float t2 = t;
    t = 0.0f;
    BoneTransform* frame1 = anim_impl->frames + frame_index1 * anim_impl->frame_stride;
    BoneTransform* frame2 = anim_impl->frames + frame_index2 * anim_impl->frame_stride;

    // Blend
    BoneTransform* blend_frame1 = nullptr;
    BoneTransform* blend_frame2 = nullptr;
    f32 blend_frame_t = 0.0f;
    f32 blend_t = 0.0f;
    bool blend_root_motion = false;
    if (layer.blend_animation) {
        AnimationImpl* blend_anim_impl = static_cast<AnimationImpl*>(layer.blend_animation);
        assert(blend_anim_impl);
        assert(GetBoneCount(animator.skeleton) == blend_anim_impl->bone_count);
        f32 blend_float_frame = layer.blend_frame_time * blend_anim_impl->frame_rate;
        i32 blend_frame_index1 = static_cast<i32>(Floor(blend_float_frame));
        blend_frame_t = blend_float_frame - static_cast<f32>(blend_frame_index1);
        blend_frame_index1 = Min(blend_frame_index1, blend_anim_impl->frame_count - 1);
        i32 blend_frame_index2 = blend_frame_index1 + 1;;
        blend_frame1 = blend_anim_impl->frames + blend_frame_index1 * blend_anim_impl->frame_stride;
        blend_frame2 = blend_anim_impl->frames + blend_frame_index2 * blend_anim_impl->frame_stride;
        blend_t = layer.blend_time / ANIMATOR_BLEND_TIME;
        blend_root_motion = IsRootMotion(layer.blend_animation);
    }

    for (int bone_index=0; bone_index<anim_impl->bone_count; bone_index++) {
        if ((layer.bone_mask & (static_cast<u64>(1) << static_cast<u64>(bone_index))) == 0)
            continue;

        BoneTransform* bt1 = frame1 + bone_index;
        BoneTransform* bt2 = frame2 + bone_index;
        BoneTransform frame_transform = Mix(*bt1, *bt2, bone_index == 0 ? t2 : t);

        Vec2 frame_position = frame_transform.position;

        if (blend_frame1) {
            assert(blend_frame2);
            BoneTransform* bbt1 = blend_frame1 + bone_index;
            BoneTransform* bbt2 = blend_frame2 + bone_index;
            BoneTransform blend_frame = Mix(*bbt1, *bbt2, blend_frame_t);
            if (blend_root_motion && bone_index == 0)
                blend_frame.position.x = 0;
            frame_transform = Mix(blend_frame, frame_transform, blend_t);
        }

        if (bone_index == 0 && animator.root_motion) {
            frame_transform.position = frame_position;

            if (setup) {
                animator.root_motion_delta = VEC2_ZERO;
            } else if (frame_index1 >= layer.frame_index) {
                animator.root_motion_delta = frame_transform.position - animator.last_root_motion;
            } else {
                BoneTransform* last_frame = anim_impl->frames + anim_impl->frame_count * anim_impl->frame_stride;
                BoneTransform* first_frame = anim_impl->frames;
                animator.root_motion_delta = last_frame->position - animator.last_root_motion;
                animator.root_motion_delta += frame_transform.position - first_frame->position;
            }
            animator.last_root_motion = frame_transform.position;
            frame_transform.position = VEC2_ZERO;
        }

        frame_transform.position += skel_impl->bones[bone_index].transform.position;
        frame_transform.position += animator.user_transforms[bone_index].position;
        frame_transform.rotation += skel_impl->bones[bone_index].transform.rotation;
        frame_transform.rotation += animator.user_transforms[bone_index].rotation;
        frame_transform.scale *= animator.user_transforms[bone_index].scale;

        animator.transforms[bone_index] = frame_transform;
    }

    layer.frame_index = frame_index1;
}

static void EvalulateFrame(Animator& animator, bool setup) {
    for (int layer_index=0; layer_index<animator.layer_count; layer_index++)
        EvalulateFrame(animator, layer_index, setup);

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

void Play(Animator& animator, Animation* animation, int layer_index, float speed, float normalized_time) {
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
    layer.loop = IsLooping(layer.animation);
    layer.playing = true;

    if ((layer.bone_mask & 0x1)) {
        animator.last_root_motion = VEC2_ZERO;
        animator.last_root_motion = VEC2_ZERO;
        animator.root_motion_delta = VEC2_ZERO;
        animator.root_motion = IsRootMotion(animation);
    }

    EvalulateFrame(animator, true);
}

void Update(Animator& animator, float time_scale) {
    float dt = GetFrameTime() * time_scale;

    for (int layer_index=0; layer_index<animator.layer_count; layer_index++) {
        AnimatorLayer& layer = GetLayer(animator, layer_index);
        if (!layer.playing || !layer.animation)
            continue;

        AnimationImpl* anim_impl = static_cast<AnimationImpl*>(layer.animation);
        layer.time += dt * layer.speed;

        if (layer.loop) {
            layer.time = fmod(layer.time, anim_impl->duration);
        } else if (layer.time >= anim_impl->duration) {
            layer.playing = false;
            layer.time = anim_impl->duration;
        }

        if (layer.blend_animation) {
            layer.blend_time += dt;
            if (layer.blend_time >= ANIMATOR_BLEND_TIME) {
                layer.blend_animation = nullptr;
                layer.blend_time = 0.0f;
                layer.blend_frame_time = 0.0f;
            }
        }
    }

    EvalulateFrame(animator, false);
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

    Update(animator, 0.0f);

    if (layer_index == 0)
        animator.root_motion_delta = VEC2_ZERO;
}

void SetBoneMask(Animator& animator, int layer_index, u64 bone_mask) {
    AnimatorLayer& layer = GetLayer(animator, layer_index);
    layer.bone_mask = bone_mask;
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
            0.0f,
            VEC2_ONE
        };
        animator.user_transforms[bone_index] = {
            VEC2_ZERO,
            0.0f,
            VEC2_ONE
        };
    }
}
