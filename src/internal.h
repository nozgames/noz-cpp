//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
#pragma once

struct RenderCamera
{
    Vec2 position;
    Vec2 size;
    Vec2 rotation;
    Vec2 padding;
};

struct RenderTransform
{
    Vec2 position;
    Vec2 scale;
    float rotation;
};

// @mesh
struct MeshVertex
{
    Vec2 position;
    Vec2 uv0;
    Vec2 normal;
    float bone;
};

struct BoneTransform
{
    Vec3 position;
    Vec3 scale;
    quat rotation;
};

typedef struct bone
{
    char* name;
    int index;
    int parentIndex;
    Mat4 world_to_local;
    Mat4 local_to_world;
    BoneTransform transform;
    float length;
    Vec3 direction;
} bone_t;

struct SamplerOptions
{
    TextureFilter filter;
    TextureClamp clamp;
};

// Function to compare sampler options
bool sampler_options_equals(SamplerOptions* a, SamplerOptions* b);

// Shader flags enum (C99 version)
typedef enum shader_flags 
{
    shader_flags_none = 0,
    shader_flags_depth_test = 1 << 0,
    shader_flags_depth_write = 1 << 1,
    shader_flags_blend = 1 << 2
} shader_flags_t;

enum VertexRegister
{
    VERTEX_REGISTER_CAMERA = 0,
    VERTEX_REGISTER_OBJECT = 1,
    VERTEX_REGISTER_BONE = 2
};

enum FragmentRegsiter
{
    FRAGMENT_REGISTER_COLOR = 0
};

enum SampleRegister
{
    SAMPLER_REGISTER_TEX0 = 0
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

// @allocator
void InitAllocator(ApplicationTraits* traits);
void ShutdownAllocator();

// @time
void InitTime();
void ShutdownTime();
void UpdateTime();

// @renderer
void BeginFrameGPU();
void EndFrameGPU();
//SDL_GPURenderPass* BeginPassGPU(bool clear, Color clear_color, bool msaa, Texture* target);
//SDL_GPURenderPass* BeginShadowPassGPU();
//SDL_GPURenderPass* BeginGammaPassGPU();
void EndRenderPassGPU();
//void BindTextureGPU(Texture* texture, SDL_GPUCommandBuffer* cb, int index);
void BindShaderGPU(Shader* shader);
//void BindMaterialGPU(Material* material, SDL_GPUCommandBuffer* cb);
void BindDefaultTextureGPU(int texture_index);

// @render_buffer
void BeginGammaPass();
void ClearRenderCommands();
//void ExecuteRenderCommands(SDL_GPUCommandBuffer* cb);

// @mesh
//void DrawMeshGPU(Mesh* mesh, SDL_GPURenderPass* pass);

// @texture
//SDL_GPUTexture* GetGPUTexture(Texture* texture);
SamplerOptions GetSamplerOptions(Texture* texture);

// @shader
struct ShaderUniformBuffer
{
    u32 size;
    u32 offset;
};

const char* GetGPUName(Shader* shader);

//SDL_GPUShader* GetGPUVertexShader(Shader* shader);
//SDL_GPUShader* GetGPUFragmentShader(Shader* shader);
//SDL_GPUCullMode GetGPUCullMode(Shader* shader);
bool IsBlendEnabled(Shader* shader);
//SDL_GPUBlendFactor GetGPUSrcBlend(Shader* shader);
//SDL_GPUBlendFactor GetGPUDstBlend(Shader* shader);
bool IsDepthTestEnabled(Shader* shader);
bool IsDepthWriteEnabled(Shader* shader);
int GetVertexUniformCount(Shader* shader);
int GetFragmentUniformCount(Shader* shader);
int GetSamplerCount(Shader* shader);
size_t GetUniformDataSize(Shader* shader);
//void PushUniformDataGPU(Shader* shader, SDL_GPUCommandBuffer* cb, u8* data);
ShaderUniformBuffer GetVertexUniformBuffer(Shader* shader, int index);
ShaderUniformBuffer GetFragmentUniformBuffer(Shader* shader, int index);

// @font

// @animation
void animation_evaluate_frame(
    Animation* animation,
    float time,
    bone_t* bones,
    size_t bone_count,
    BoneTransform* transforms,
    size_t transform_count);

// @helpers
/*
inline SDL_FColor ColorToSDL(Color color)
{
    SDL_FColor result;
    result.r = color.r;
    result.g = color.g;
    result.b = color.b;
    result.a = color.a;
    return result;
}
*/

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
