//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

struct Animation : Asset {};

// @animation
constexpr int MAX_ANIMATION_FRAMES = 64;
constexpr int ANIMATION_FRAME_RATE = 12;
constexpr float ANIMATION_FRAME_TIME = 1.0f / ANIMATION_FRAME_RATE;

typedef u32 AnimationFlags;
constexpr AnimationFlags ANIMATION_FLAG_NONE = 0;
constexpr AnimationFlags ANIMATION_FLAG_LOOPING = 1 << 0;
constexpr AnimationFlags ANIMATION_FLAG_ROOT_MOTION = 1 << 1;

struct BoneTransform {
    Vec2 position;
    Vec2 scale;
    float rotation;
};

struct Bone {
    const Name* name;
    i32 index;
    i32 parent_index;
    BoneTransform transform;
    Mat3 bind_pose;
};

struct AnimationEvent {
    int frame;
    int id;
};

extern Animation* CreateAnimation(
    Allocator* allocator,
    Skeleton* skeleton,
    int frame_count,
    int frame_rate,
    BoneTransform* transforms,
    int transform_count,
    AnimationEvent* events,
    int event_count,
    AnimationFlags flags,
    const Name* name=NAME_NONE);

extern void AddEvent(Animation* animation, int frame, int event_id);
extern void AddEvents(Animation* animation, AnimationEvent* events, int event_count);
extern void AddEvents(Animation* animation, int event, int* frames, int frame_count);

extern int GetBoneCount(Animation* animation);
extern bool IsRootMotion(Animation* animation);
extern bool IsLooping(Animation* animation);
extern float GetDuration(Animation* animation);

inline BoneTransform Mix(const BoneTransform& a, const BoneTransform& b, float t) {
    BoneTransform result;
    result.position = Mix(a.position, b.position, t);
    result.rotation = MixAngle(a.rotation, b.rotation, t);
    result.scale = Mix(a.scale, b.scale, t);
    return result;
}

inline Mat3 ToMat3(const BoneTransform& bt) {
    return TRS(bt.position, bt.rotation, bt.scale);
}

constexpr int MAX_ANIMATION_LAYERS = 2;
constexpr int ANIMATION_MAX_EVENTS = 4;

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
    float root_motion_delta;
    float last_root_motion;
    Mat3 bones[MAX_BONES];
    BoneTransform transforms[MAX_BONES];
    int events[ANIMATION_MAX_EVENTS];
    int event_count;
};

extern void Init(Animator& animator, Skeleton* skeleton, int layer_count=1);
extern void SetBoneMask(Animator& animator, int layer_index, u64 bone_mask);
extern void Play(Animator& animator, Animation* animation, int layer_index=0, float speed=1.0f, float normalized_time=0.0f, bool loop=true);
extern void Stop(Animator& animator);
extern void Stop(Animator& animatorl, int layer_index);
extern void Update(Animator& animator, float time_scale=1.0f);
extern bool IsLooping(Animator& animator);
inline bool HasEvents(Animator& animator) { return animator.event_count > 0; }
extern int GetEvent(Animator& animator);
inline int GetFrameIndex(Animator& animator, int layer_index=0) { return animator.layers[layer_index].frame_index; }
extern void SetNormalizedTime(Animator& animator, int layer_index, float normalized_time);
extern void SetFrame(Animator& animator, int layer_index, int frame_index);
inline Skeleton* GetSkeleton(Animator& animator) {
    return animator.skeleton;
}
inline AnimatorLayer& GetLayer(Animator& animator, int layer_index) {
    assert(layer_index >= 0 && layer_index < animator.layer_count);
    return animator.layers[layer_index];
}
inline bool IsPlaying(Animator& animator, int layer_index, Animation* animation) {
    return GetLayer(animator, layer_index).animation == animation;
}
inline bool IsPlaying(Animator& animator, int layer_index=0) {
    if (animator.layer_count == 0) return false;
    return GetLayer(animator, layer_index).playing;
}
inline bool IsLooping(AnimationFlags flags) {
    return (flags & ANIMATION_FLAG_LOOPING) != 0;
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

extern void BindSkeleton(Skeleton* skeleton);
extern void BindSkeleton(Animator& animator);

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
    u64 bone_mask;
    float value;
};

extern void Init(BlendTree& blend_tree, Skeleton* skeleton, int blend_count, u64 bone_mask=0xFFFFFFFFFFFFFFFF);
extern void Play(BlendTree& blend_tree, int blend_index, float value, Animation* animation, float speed=1.0f, float normalized_time=0.0f);
extern void Stop(BlendTree& blend_tree);
extern void SetValue(BlendTree& blend_tree, float value);
extern void Update(BlendTree& blend_tree, float time_scale, Animator& animator);

inline bool IsPlaying(BlendTree& blend_tree) {
    return IsPlaying(blend_tree.blends[0].animator);
}

extern Animation** ANIMATION;
extern int ANIMATION_COUNT;