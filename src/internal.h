//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
#pragma once

#include <SDL3/SDL.h>
#include <noz/noz.h>
#include <noz/color.h>

#include "editor/editor_client.h"

// @mesh
typedef struct mesh_vertex
{
    Vec3 position;
    Vec2 uv0;
    Vec3 normal;
    float bone;
} mesh_vertex;


typedef struct bone_transform
{
    Vec3 position;
    Vec3 scale;
    quat rotation;
} bone_transform_t;

typedef struct bone
{
    char* name;
    int index;
    int parentIndex;
    Mat4 world_to_local;
    Mat4 local_to_world;
    bone_transform_t transform;
    float length;
    Vec3 direction;
} bone_t;

struct SamplerOptions
{
    TextureFilter min_filter;
    TextureFilter mag_filter;
    TextureClamp clamp_u;
    TextureClamp clamp_v;
    TextureClamp clamp_w;
    SDL_GPUCompareOp compare_op;
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

// Register enums (C99 versions)
typedef enum vertex_register 
{
    vertex_register_camera = 0,
    vertex_register_object = 1,
    vertex_register_bone = 2,
    vertex_register_user0 = 3,
    vertex_register_user1 = 4,
    vertex_register_user2 = 5,
    vertex_register_count
} vertex_register_t;

typedef enum fragment_register 
{
    fragment_register_color = 0,
    fragment_register_light = 1,
    fragment_register_user0 = 2,
    fragment_register_user1 = 3,
    fragment_register_user2 = 4,
    fragment_register_count
} fragment_register_t;

typedef enum sampler_register 
{
    sampler_register_shadow_map = 0,
    sampler_register_user0 = 1,
    sampler_register_user1 = 2,
    sampler_register_user2 = 3,
    sampler_register_count
} sampler_register_t;

// Function to convert texture format to SDL format
SDL_GPUTextureFormat texture_format_to_sdl(TextureFormat format);

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
void InitRenderer(RendererTraits* traits, SDL_Window* window);
void ShutdownRenderer();
void BeginFrameGPU();
void EndFrameGPU();
SDL_GPURenderPass* BeginPassGPU(bool clear, Color clear_color, bool msaa, Texture* target);
SDL_GPURenderPass* BeginShadowPassGPU();
SDL_GPURenderPass* BeginGammaPassGPU();
void EndRenderPassGPU();
void BindTextureGPU(Texture* texture, SDL_GPUCommandBuffer* cb, int index);
void BindShaderGPU(Shader* shader);
void BindMaterialGPU(Material* material, SDL_GPUCommandBuffer* cb);
void BindDefaultTextureGPU(int texture_index);

// @render_buffer
void InitRenderBuffer(RendererTraits* traits);
void ShutdownRenderBuffer();
void BeginGammaPass();
void ClearRenderCommands();
void ExecuteRenderCommands(SDL_GPUCommandBuffer* cb);

// @sampler_factory
void InitSamplerFactory(RendererTraits* traits, SDL_GPUDevice* device);
void ShutdownSamplerFactory();
SDL_GPUSampler* GetGPUSampler(Texture* texture);

// @pipeline_factory
void InitPipelineFactory(RendererTraits* traits, SDL_Window* window, SDL_GPUDevice* device);
void ShutdownPipelineFactory();
SDL_GPUGraphicsPipeline* GetGPUPipeline(Shader* shader, bool msaa, bool shadow);

// @mesh
void InitMesh(RendererTraits* traits, SDL_GPUDevice* device);
void ShutdownMesh();
void DrawMeshGPU(Mesh* mesh, SDL_GPURenderPass* pass);

// @texture
void InitTexture(RendererTraits* traits, SDL_GPUDevice* device);
void ShutdownTexture();
SDL_GPUTexture* GetGPUTexture(Texture* texture);
SamplerOptions GetSamplerOptions(Texture* texture);

// @shader
struct ShaderUniformBuffer
{
    u32 size;
    u32 offset;
};

void InitShader(RendererTraits* traits, SDL_GPUDevice* device);
void ShutdownShader();
const char* GetGPUName(Shader* shader);

SDL_GPUShader* GetGPUVertexShader(Shader* shader);
SDL_GPUShader* GetGPUFragmentShader(Shader* shader);
SDL_GPUCullMode GetGPUCullMode(Shader* shader);
bool IsBlendEnabled(Shader* shader);
SDL_GPUBlendFactor GetGPUSrcBlend(Shader* shader);
SDL_GPUBlendFactor GetGPUDstBlend(Shader* shader);
bool IsDepthTestEnabled(Shader* shader);
bool IsDepthWriteEnabled(Shader* shader);
int GetVertexUniformCount(Shader* shader);
int GetFragmentUniformCount(Shader* shader);
int GetSamplerCount(Shader* shader);
size_t GetUniformDataSize(Shader* shader);
void PushUniformDataGPU(Shader* shader, SDL_GPUCommandBuffer* cb, u8* data);
ShaderUniformBuffer GetVertexUniformBuffer(Shader* shader, int index);
ShaderUniformBuffer GetFragmentUniformBuffer(Shader* shader, int index);

// @font
void InitFont(RendererTraits* traits, SDL_GPUDevice* device);
void ShutdownFont();

// @animation
void animation_evaluate_frame(
    Animation* animation,
    float time,
    bone_t* bones,
    size_t bone_count,
    bone_transform_t* transforms,
    size_t transform_count);

// @helpers
inline SDL_FColor ColorToSDL(Color color)
{
    SDL_FColor result;
    result.r = color.r;
    result.g = color.g;
    result.b = color.b;
    result.a = color.a;
    return result;
}

// @input
void InitInput();
void ShutdownInput();
void UpdateInput();

// @helper
InputCode ScanCodeToInputCode(SDL_Scancode scancode);
InputCode InputCodeFromMouseButton(int button);
SDL_Scancode InputCodeToScanCode(InputCode code);

// @physics
void InitPhysics();
void ShutdownPhysics();
void UpdatePhysics();

// @text_engine
struct TextMesh : Object {};

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
