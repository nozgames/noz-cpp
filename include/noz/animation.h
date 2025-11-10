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

constexpr int MAX_ANIMATION_LAYERS = 2;

struct AnimatorLayer {
    Animation* animation;
    Animation* blend_animation;
    int frame_index;
    float blend_time;
    float blend_frame_time;
    float time;
    float speed;
    u64 bone_mask;
    bool blend_loop;
    bool loop;
    bool playing;
};

// @animator
struct Animator {
    Skeleton* skeleton;
    AnimatorLayer layers[MAX_ANIMATION_LAYERS];
    int layer_count;
    bool root_motion;
    Vec2 root_motion_delta;
    Vec2 last_root_motion;
    Mat3 bones[MAX_BONES];
    BoneTransform transforms[MAX_BONES];
    BoneTransform user_transforms[MAX_BONES];
};

extern void Init(Animator& animator, Skeleton* skeleton, int layer_count=1);
extern void SetBoneMask(Animator& animator, int layer_index, u64 bone_mask);
extern void Play(Animator& animator, Animation* animation, int layer_index=0, float speed=1.0f);
extern void Stop(Animator& animator);
extern void Stop(Animator& animatorl, int layer_index);
extern void Update(Animator& animator, float time_scale=1.0f);
extern bool IsLooping(Animator& animator);
extern float GetNormalizedTime(Animator& animator, int layer_index);
extern void SetNormalizedTime(Animator& animator, int layer_index, float normalized_time);
inline AnimatorLayer& GetLayer(Animator& animator, int layer_index) {
    assert(layer_index >= 0 && layer_index < animator.layer_count);
    return animator.layers[layer_index];
}
inline bool IsPlaying(Animator& animator, int layer_index, Animation* animation) {
    return GetLayer(animator, layer_index).animation == animation;
}
inline bool IsPlaying(Animator& animator, int layer_index=0) {
    return GetLayer(animator, layer_index).playing;
}
inline bool IsLooping(Animator& animator, int layer_index=0) {
    return GetLayer(animator, layer_index).loop;
}
inline bool IsBlending(Animator& animator, int layer_index=0) {
    return GetLayer(animator, layer_index).blend_animation != nullptr;
}
inline float GetTime(Animator& animator, int layer_index=0) {
    return GetLayer(animator, layer_index).time;
}

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

extern void Init(BlendTree& blend_tree, Skeleton* skeleton, int blend_count, u64 bone_mask=0xFFFFFFFFFFFFFFFF);
extern void Play(BlendTree& blend_tree, int blend_index, float value, Animation* animation, float speed=1.0f);
extern void SetValue(BlendTree& blend_tree, float value);
extern void Update(BlendTree& blend_tree, float time_scale, Animator& animator, int layer_index=0);

extern Animation** ANIMATION;
