//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
#pragma once

struct SamplerOptions
{
    TextureFilter filter;
    TextureClamp clamp;
};

// Function to compare sampler options
bool sampler_options_equals(SamplerOptions* a, SamplerOptions* b);

typedef u32 ShaderFlags;
constexpr ShaderFlags SHADER_FLAGS_NONE = 0;
constexpr ShaderFlags SHADER_FLAGS_BLEND = 1 << 0;

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

typedef enum animation_track_type
{
    animation_track_type_translation = 0, // position (vec3)
    animation_track_type_rotation = 1,    // rotation (quat)
    animation_track_type_scale = 2        // scale (vec3)
} animation_track_type_t;

typedef struct animation_track
{
    uint8_t bone;
    animation_track_type_t type;
    int data_offset;
} animation_track_t;

// @render_buffer

// @animation
// void animation_evaluate_frame(
//     Animation* animation,
//     float time,
//     bone_t* bones,
//     size_t bone_count,
//     BoneTransform* transforms,
//     size_t transform_count);

// @input
void InitInput();
void ShutdownInput();
void UpdateInput();

// @helper
//InputCode ScanCodeToInputCode(SDL_Scancode scancode);
InputCode InputCodeFromMouseButton(int button);
//SDL_Scancode InputCodeToScanCode(InputCode code);

// @physics
void InitPhysics();
void ShutdownPhysics();
void UpdatePhysics();

// @text_engine
struct TextMesh {};

struct TextRequest
{
    text_t text;
    Font* font;
    int font_size;
};

TextMesh* CreateTextMesh(Allocator* allocator, const TextRequest& request);
Vec2 MeasureText(const text_t& text, Font* font, float font_size);
Mesh* GetMesh(TextMesh* tm);
Material* GetMaterial(TextMesh* tm);
Vec2 GetSize(TextMesh* tm);


// @animation
struct AnimationBone
{
    u8 index;
};

struct AnimationImpl : Animation
{
    int bone_count;
    int frame_count;
    int frame_stride;
    float duration;
    AnimationBone* bones;
    BoneTransform* frames;
};

// @skeleton
struct Bone
{
    const Name* name;
    i32 index;
    i32 parent_index;
    Transform transform;
};

struct SkeletonImpl : Skeleton
{
    int bone_count;
    Bone* bones;
};

// @tween
extern void InitTween();
extern void ShutdownTween();
extern void UpdateTweens();
