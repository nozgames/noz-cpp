//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//


static void EvalulateFrame(Animator& animator)
{
    assert(animator.skeleton);
    assert(animator.animation);
    assert(GetBoneCount(animator.skeleton) == GetBoneCount(animator.animation));

    AnimationImpl* anim_impl = (AnimationImpl*)animator.animation;
    SkeletonImpl* skel_impl = (SkeletonImpl*)animator.skeleton;

    f32 float_frame = animator.time * ANIMATION_FRAME_RATE;
    assert(float_frame < anim_impl->frame_count);
    i32 frame1 = static_cast<i32>(Floor(float_frame));
    i32 frame2 = (frame1 + 1) % anim_impl->frame_count;
    f32 t = float_frame - static_cast<f32>(frame1);
    assert(t >= 0.0f && t < 1.0f);
    BoneTransform* frames = anim_impl->frames;

    for (int i=0; i<anim_impl->bone_count; i++)
    {
        auto& bt1 = frames[frame1 * anim_impl->frame_stride + i];
        auto& bt2 = frames[frame2 * anim_impl->frame_stride + i];

        Vec2 position = Mix(bt1.position, bt2.position, t) + skel_impl->bones[i].transform.position;
        float rotation = Mix(bt1.rotation, bt2.rotation, t) + skel_impl->bones[i].transform.rotation;
        float scale = Mix(bt1.scale, bt2.scale, t);

        animator.bones[i] = TRS(position, rotation, Vec2{scale, scale});
    }

    for (int i=1; i<anim_impl->bone_count; i++)
    {
        int parent_index = ((SkeletonImpl*)animator.skeleton)->bones[i].parent_index;
        animator.bones[i] = animator.bones[parent_index] * animator.bones[i];
    }

    animator.last_frame = frame1;
}

void Init(Animator& animator, Skeleton* skeleton)
{
    animator.skeleton = skeleton;
    animator.animation = nullptr;
    animator.time = 0.0f;
    animator.speed = 1.0f;
    animator.last_frame = -1;
    for (int i=0; i<GetBoneCount(skeleton); i++)
        animator.bones[i] = MAT3_IDENTITY;
}

void Stop(Animator& animator)
{
    animator.animation = nullptr;
}

void Play(Animator& animator, Animation* animation, float speed, bool loop)
{
    animator.animation = animation;
    animator.speed = speed;
    animator.time = 0.0f;
    animator.loop = loop;
    EvalulateFrame(animator);
}

void Update(Animator& animator, float time_scale)
{
    float duration = ((AnimationImpl*)animator.animation)->duration;

    animator.time += GetFrameTime() * time_scale * animator.speed;
    if (animator.loop)
        animator.time = fmod(animator.time, duration + ANIMATION_FRAME_RATE_INV);
    else
        animator.time = Min(animator.time, duration);

    EvalulateFrame(animator);
}

bool IsPlaying(Animator& animator)
{
    return animator.animation != nullptr;
}

int GetFrame(Animator& animator)
{
    return animator.last_frame;
}

float GetTime(Animator& animator)
{
    return animator.time;
}

float GetNormalizedTime(Animator& animator)
{
    if (!animator.animation)
        return 0.0f;

    return animator.time / ((AnimationImpl*)animator.animation)->duration;
}
