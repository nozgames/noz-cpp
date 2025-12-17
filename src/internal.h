//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
#pragma once

struct SamplerOptions {
    TextureFilter filter;
    TextureClamp clamp;
};

// Function to compare sampler options
bool sampler_options_equals(SamplerOptions* a, SamplerOptions* b);

typedef u32 ShaderFlags;
constexpr ShaderFlags SHADER_FLAGS_NONE = 0;
constexpr ShaderFlags SHADER_FLAGS_BLEND = 1 << 0;
constexpr ShaderFlags SHADER_FLAGS_DEPTH = 1 << 1;
constexpr ShaderFlags SHADER_FLAGS_DEPTH_LESS = 1 << 2;
constexpr ShaderFlags SHADER_FLAGS_POSTPROCESS = 1 << 3;
constexpr ShaderFlags SHADER_FLAGS_UI_COMPOSITE = 1 << 4;
constexpr ShaderFlags SHADER_FLAGS_PREMULTIPLIED_ALPHA = 1 << 5;

enum VertexRegister
{
    VERTEX_REGISTER_CAMERA = 0,
    VERTEX_REGISTER_OBJECT = 1,
    //VERTEX_REGISTER_BONE = 2,
    VERTEX_REGISTER_COUNT
};

enum FragmentRegsiter
{
    FRAGMENT_REGISTER_COLOR = 0,
    FRAGMENT_REGISTER_COUNT
};

enum SampleRegister
{
    SAMPLER_REGISTER_TEX0 = 0,
    SAMPLER_REGISTER_COUNT
};

// @render
void BeginUIPass();

// @input
void InitInput();
void ShutdownInput();
void UpdateInput();

// @physics
void InitPhysics();
void ShutdownPhysics();
void UpdatePhysics();

// @animation
struct AnimationBone {
    u8 index;
};

struct AnimationFrame {
    int transform0;
    int transform1;
    int event;
    float fraction0;
    float fraction1;
    float root_motion0;
    float root_motion1;
};

struct AnimationImpl : Animation {
    int bone_count;
    int transform_count;
    int transform_stride;
    int frame_count;
    int frame_rate;
    float frame_rate_inv;
    float duration;
    AnimationFlags flags;
    AnimationBone* bones;
    BoneTransform* transforms;
    AnimationFrame* frames;
};

// @skeleton
struct SkeletonImpl : Skeleton {
    int bone_count;
    Bone* bones;
};

// @tween
extern void InitTween();
extern void ShutdownTween();

