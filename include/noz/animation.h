//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Animation : Asset {};

// @animation
constexpr int MAX_ANIMATION_FRAMES = 64;
constexpr int ANIMATION_FRAME_RATE = 12;

extern int GetBoneCount(Animation* animation);

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
    float time;
    float speed;
    int last_frame;
    bool loop;
    Mat3 bones[MAX_BONES];
    BoneTransform user_transforms[MAX_BONES];
};

extern void Init(Animator& animator, Skeleton* skeleton);
extern void Play(Animator& animator, Animation* animation, float speed=1.0f, bool loop=false);
extern void Stop(Animator& animator);
extern void Update(Animator& animator, float time_scale=1.0f);
extern bool IsPlaying(Animator& animator);
extern bool IsLooping(Animator& animator);
extern int GetFrame(Animator& animator);
extern float GetTime(Animator& animator);
extern float GetNormalizedTime(Animator& animator);
extern void SetNormalizedTime(Animator& animator, float normalized_time);


extern Animation** ANIMATION;
