//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

constexpr float ANIMATOR_BLEND_TIME = 0.05f;

static void EvalulateFrame(Animator& animator) {
    assert(animator.skeleton);
    assert(animator.animation);
    assert(GetBoneCount(animator.skeleton) == GetBoneCount(animator.animation));

    AnimationImpl* anim_impl = static_cast<AnimationImpl*>(animator.animation);
    SkeletonImpl* skel_impl = static_cast<SkeletonImpl*>(animator.skeleton);

    f32 float_frame = animator.time * anim_impl->frame_rate;
    assert(float_frame < anim_impl->frame_count);
    i32 frame_index1 = static_cast<i32>(Floor(float_frame));
    i32 frame_index2 = animator.loop ? ((frame_index1 + 1) % anim_impl->frame_count) : frame_index1;
    f32 t = float_frame - static_cast<f32>(frame_index1);
    assert(t >= 0.0f && t < 1.0f);
    BoneTransform* frame1 = anim_impl->frames + frame_index1 * anim_impl->frame_stride;
    BoneTransform* frame2 = anim_impl->frames + frame_index2 * anim_impl->frame_stride;


    BoneTransform* blend_frame1 = nullptr;
    BoneTransform* blend_frame2 = nullptr;
    f32 blend_frame_t = 0.0f;
    f32 blend_t = 0.0f;
    if (animator.blend_animation) {
        AnimationImpl* blend_anim_impl = static_cast<AnimationImpl*>(animator.blend_animation);
        assert(blend_anim_impl);
        assert(GetBoneCount(animator.skeleton) == blend_anim_impl->bone_count);
        f32 blend_float_frame = animator.blend_frame_time * blend_anim_impl->frame_rate;
        i32 blend_frame_index1 = static_cast<i32>(Floor(blend_float_frame));
        i32 blend_frame_index2 = animator.blend_loop ? ((blend_frame_index1 + 1) % blend_anim_impl->frame_count) : blend_frame_index1;
        blend_frame1 = blend_anim_impl->frames + blend_frame_index1 * blend_anim_impl->frame_stride;
        blend_frame2 = blend_anim_impl->frames + blend_frame_index2 * blend_anim_impl->frame_stride;
        blend_frame_t = blend_float_frame - static_cast<f32>(blend_frame_index1);
        blend_t = animator.blend_time / ANIMATOR_BLEND_TIME;
    }

    for (int bone_index=0; bone_index<anim_impl->bone_count; bone_index++) {
        BoneTransform* bt1 = frame1 + bone_index;
        BoneTransform* bt2 = frame2 + bone_index;
        BoneTransform frame_transform = Mix(*bt1, *bt2, t);

        if (blend_frame1) {
            assert(blend_frame2);
            BoneTransform* bbt1 = blend_frame1 + bone_index;
            BoneTransform* bbt2 = blend_frame2 + bone_index;
            BoneTransform blend_frame = Mix(*bbt1, *bbt2, blend_frame_t);
            frame_transform = Mix(blend_frame, frame_transform, blend_t);
        }

        frame_transform.position += skel_impl->bones[bone_index].transform.position;
        frame_transform.position += animator.user_transforms[bone_index].position;
        frame_transform.rotation += animator.user_transforms[bone_index].rotation;
        frame_transform.scale *= animator.user_transforms[bone_index].scale;

        animator.bones[bone_index] = ToMat3(frame_transform);
    }

    for (int bone_index=1; bone_index<anim_impl->bone_count; bone_index++) {
        int parent_index = skel_impl->bones[bone_index].parent_index;
        animator.bones[bone_index] = animator.bones[parent_index] * animator.bones[bone_index];
    }
}

void Stop(Animator& animator) {
    animator = {};
    animator.animation = nullptr;
    animator.blend_animation = nullptr;
}

void Play(Animator& animator, Animation* animation, float speed, bool loop) {
    animator.blend_animation = animator.animation;
    animator.blend_time = 0.0f;
    animator.blend_frame_time = animator.time;
    animator.blend_loop = animator.loop;
    animator.animation = animation;
    animator.speed = speed;
    animator.time = 0.0f;
    animator.loop = loop;
    EvalulateFrame(animator);
}

void Update(Animator& animator, float time_scale) {
    AnimationImpl* anim_impl = static_cast<AnimationImpl*>(animator.animation);

    float dt = GetFrameTime() * time_scale;
    animator.time += dt * animator.speed;

    if (animator.loop)
        animator.time = fmod(animator.time, anim_impl->duration + anim_impl->frame_rate_inv);
    else
        animator.time = Min(animator.time, anim_impl->duration);

    if (animator.blend_animation) {
        animator.blend_time += dt;
        if (animator.blend_time >= ANIMATOR_BLEND_TIME) {
            animator.blend_animation = nullptr;
            animator.blend_time = 0.0f;
            animator.blend_frame_time = 0.0f;
        }
    }

    EvalulateFrame(animator);
}

bool IsPlaying(Animator& animator)
{
    return animator.animation != nullptr && (animator.loop || animator.time < ((AnimationImpl*)animator.animation)->duration);
}

bool IsLooping(Animator& animator)
{
    return animator.loop;
}

float GetTime(Animator& animator) {
    return animator.time;
}

float GetNormalizedTime(Animator& animator) {
    if (!animator.animation)
        return 0.0f;

    return animator.time / ((AnimationImpl*)animator.animation)->duration;
}

void SetNormalizedTime(Animator& animator, float normalized_time) {
    if (animator.animation == nullptr)
        return;

    animator.time = normalized_time * ((AnimationImpl*)animator.animation)->duration;

    Update(animator, 0.0f);
}

void Init(Animator& animator, Skeleton* skeleton) {
    animator = {};
    animator.skeleton = skeleton;
    animator.speed = 1.0f;

    int bone_count = GetBoneCount(skeleton);
    for (int bone_index=0; bone_index<bone_count; bone_index++) {
        animator.bones[bone_index] = MAT3_IDENTITY;
        animator.user_transforms[bone_index] = {
            VEC2_ZERO,
            0.0f,
            VEC2_ONE
        };
    }
}
