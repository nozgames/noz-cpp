//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

constexpr float ANIMATOR_BLEND_TIME = 0.1f;

static void EvalulateFrame(Animator& animator) {
    assert(animator.skeleton);
    assert(animator.animation);
    assert(GetBoneCount(animator.skeleton) == GetBoneCount(animator.animation));

    AnimationImpl* anim_impl = (AnimationImpl*)animator.animation;
    SkeletonImpl* skel_impl = (SkeletonImpl*)animator.skeleton;

    f32 float_frame = animator.time * anim_impl->frame_rate;
    assert(float_frame < anim_impl->frame_count);
    i32 frame1 = static_cast<i32>(Floor(float_frame));
    i32 frame2 = (frame1 + 1) % anim_impl->frame_count;
    f32 t = float_frame - static_cast<f32>(frame1);
    assert(t >= 0.0f && t < 1.0f);
    BoneTransform* frames = anim_impl->frames;

    BoneTransform* blend_frames1 = nullptr;
    BoneTransform* blend_frames2 = nullptr;
    f32 blend_frame_t = 0.0f;
    f32 blend_t = 0.0f;
    if (animator.blend_animation) {
        AnimationImpl* blend_anim_impl = (AnimationImpl*)animator.blend_animation;
        assert(GetBoneCount(animator.skeleton) == blend_anim_impl->bone_count);


        f32 blend_float_frame = animator.blend_time * blend_anim_impl->frame_rate;
        i32 blend_frame1 = static_cast<i32>(Floor(blend_float_frame));
        i32 blend_frame2 = (blend_frame1 + 1) % blend_anim_impl->frame_count;
        blend_frames1 = blend_anim_impl->frames + blend_frame1 * blend_anim_impl->frame_stride;
        blend_frames2 = blend_anim_impl->frames + blend_frame2 * blend_anim_impl->frame_stride;
        blend_frame_t = blend_float_frame - static_cast<f32>(blend_frame1);
    }

    for (int bone_index=0; bone_index<anim_impl->bone_count; bone_index++) {
        auto& bt1 = frames[frame1 * anim_impl->frame_stride + bone_index];
        auto& bt2 = frames[frame2 * anim_impl->frame_stride + bone_index];

        BoneTransform frame_transform = Mix(bt1, bt2, t);

        if (blend_frames1) {
            auto& bbt1 = *(blend_frames1 + bone_index);
            auto& bbt2 = *(blend_frames2 + bone_index);
            frame_transform = Mix(frame_transform, Mix(bbt1, bbt2, blend_frame_t), blend_t);
        }

        animator.bones[bone_index] = ToMat3(frame_transform);
    }

    for (int i=1; i<anim_impl->bone_count; i++) {
        int parent_index = skel_impl->bones[i].parent_index;
        animator.bones[i] = animator.bones[parent_index] * animator.bones[i];
    }

    animator.last_frame = frame1;
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

int GetFrame(Animator& animator) {
    return animator.last_frame;
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
    animator.last_frame = -1;

    Update(animator, 0.0f);
}

void Init(Animator& animator, Skeleton* skeleton) {
    animator = {};
    animator.skeleton = skeleton;
    animator.speed = 1.0f;
    animator.last_frame = -1;

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
