//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Animation : Asset {};

// @animation
constexpr int MAX_ANIMATION_FRAMES = 64;
constexpr int ANIMATION_FRAME_RATE = 12;

typedef u32 AnimationFlags;
constexpr AnimationFlags ANIMATION_FLAG_NONE = 0;
constexpr AnimationFlags ANIMATION_FLAG_LOOPING = 1 << 0;
constexpr AnimationFlags ANIMATION_FLAG_ROOT_MOTION = 1 << 1;

extern int GetBoneCount(Animation* animation);
extern bool IsRootMotion(Animation* animation);
extern bool IsLooping(Animation* animation);

// @bone_transform
struct BoneTransform {
    Vec2 position;
    float rotation;
    Vec2 scale;
};

inline BoneTransform Mix(const BoneTransform& a, const BoneTransform& b, float t) {
    BoneTransform result;
    result.position = Mix(a.position, b.position, t);
    result.rotation = Mix(a.rotation, b.rotation, t);
    result.scale = Mix(a.scale, b.scale, t);
    return result;
}

inline Mat3 ToMat3(const BoneTransform& bt) {
    return TRS(bt.position, bt.rotation, bt.scale);
}

// @animator
struct Animator {
    Skeleton* skeleton;
    Animation* animation;
    Animation* blend_animation;
    float blend_time;
    float blend_frame_time;
    bool blend_loop;
    float time;
    float speed;
    bool loop;
    bool root_motion;
    int frame_index;
    Vec2 root_motion_delta;
    Vec2 last_root_motion;
    Mat3 bones[MAX_BONES];
    BoneTransform transforms[MAX_BONES];
    BoneTransform user_transforms[MAX_BONES];
};

extern void Init(Animator& animator, Skeleton* skeleton);
extern void Play(Animator& animator, Animation* animation, float speed=1.0f, bool loop=false);
extern void Stop(Animator& animator);
extern void Update(Animator& animator, float time_scale=1.0f);
extern bool IsPlaying(Animator& animator);
extern bool IsLooping(Animator& animator);
extern float GetTime(Animator& animator);
extern float GetNormalizedTime(Animator& animator);
extern void SetNormalizedTime(Animator& animator, float normalized_time);

// @blend_tree
constexpr int MAX_BLEND_TREE_BLENDS = 3;

struct BlendTreeBlend {
    Animator animator;
    float value;
};

struct BlendTree {
    Skeleton* skeleton;
    BlendTreeBlend blends[MAX_BLEND_TREE_BLENDS];
    int blend_count;
    float value;
};

extern void Init(BlendTree& blend_tree, Skeleton* skeleton, int blend_count);
extern void Play(BlendTree& blend_tree, int blend_index, float value, Animation* animation, float speed=1.0f, bool loop=false);
extern void SetValue(BlendTree& blend_tree, float value);
extern void Update(BlendTree& blend_tree, float time_scale, Animator& animator);

extern Animation** ANIMATION;
