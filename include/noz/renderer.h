//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// @types
struct Camera;
struct Texture : Object {};
struct Material : Object {};
struct Mesh : Object {};
struct Font : Object {};
struct Shader : Object {};
struct MeshBuilder : Object {};
struct Animation : Object {};

// @renderer_traits
struct RendererTraits
{
    size_t max_textures;
    size_t max_shaders;
    size_t max_samplers;
    size_t max_pipelines;
    size_t max_meshes;
    size_t max_fonts;
    size_t max_frame_commands;
    size_t max_frame_objects;
    size_t max_frame_transforms;
    uint32_t shadow_map_size;
};

// @texture

enum TextureFilter
{
    TEXTURE_FILTER_NEAREST,
    TEXTURE_FILTER_LINEAR
};

enum TextureClamp
{
    TEXTURE_CLAMP_REPEAT,
    TEXTURE_CLAMP_CLAMP,
    TEXTURE_CLAMP_REPEAT_MIRRORED
};

enum TextureFormat
{
    TEXTURE_FORMAT_RGBA8,
    TEXTURE_FORMAT_RGBA16F,
    TEXTURE_FORMAT_R8
};

Texture* CreateTexture(Allocator* allocator, void* data, size_t width, size_t height, TextureFormat format, const name_t* name);
Texture* CreateTexture(Allocator* allocator, int width, int height, TextureFormat format, const name_t* name);
int GetBytesPerPixel(TextureFormat format);
ivec2 GetSize(Texture* texture);

// @material
Material* CreateMaterial(Allocator* allocator, Shader* shader);
Shader* GetShader(Material* material);
void SetTexture(Material* material, Texture* texture, size_t index=0);

// @mesh
Mesh* CreateMesh(
    Allocator* allocator,
    size_t vertex_count,
    vec3* positions,
    vec3* normals,
    vec2* uvs,
    u8* bone_indices,
    size_t index_count,
    u16* indices,
    const name_t* name);
Mesh* CreateMesh(Allocator* allocator, MeshBuilder* builder, const name_t* name);

// @mesh_builder
MeshBuilder* CreateMeshBuilder(Allocator* allocator, int max_vertices, int max_indices);
void Clear(MeshBuilder* builder);
vec3* GetPositions(MeshBuilder* builder);
vec3* GetNormals(MeshBuilder* builder);
vec2* GetUvs(MeshBuilder* builder);
u8* GetBoneIndices(MeshBuilder* builder);
u16* GetIndices(MeshBuilder* builder);
size_t GetVertexCount(MeshBuilder* builder);
size_t GetIndexCount(MeshBuilder* builder);
void AddIndex(MeshBuilder* builder, uint16_t index);
void AddTriangle(MeshBuilder* builder, uint16_t a, uint16_t b, uint16_t c);
void AddTriangle(MeshBuilder* builder, vec3 a, vec3 b, vec3 c, uint8_t bone_index);
void AddPyramid(MeshBuilder* builder, vec3 start, vec3 end, float size, uint8_t bone_index);
void AddCube(MeshBuilder* builder, vec3 center, vec3 size, uint8_t bone_index);
void AddRaw(
    MeshBuilder* builder,
    size_t vertex_count,
    vec3* positions,
    vec3* normals,
    vec2* uv0,
    uint8_t bone_index,
    size_t index_count,
    uint16_t* indices);
void AddQuad(
    MeshBuilder* builder,
    vec3 forward,
    vec3 right,
    float width,
    float height,
    vec2 color_uv);
void AddQuad(
    MeshBuilder* builder,
    vec3 a,
    vec3 b,
    vec3 c,
    vec3 d,
    vec2 uv_color,
    vec3 normal,
    uint8_t bone_index=0);
void AddVertex(
    MeshBuilder* builder,
    vec3 position,
    vec3 normal,
    vec2 uv,
    uint8_t bone_index=0);

// @render_buffer
void ClearRenderCommands();
void BeginRenderPass(bool clear, color_t clear_color, bool msaa, Texture* target);
void BeginShadowPass(mat4 light_view, mat4 light_projection);
void BindDefaultTexture(int texture_index);
void BindColor(color_t color);
void BindCamera(Camera* camera);
void BindCamera(const mat4& view, const mat4& projection);
void BindTransform(const mat4& transform);
void BindMaterial(Material* material);
void DrawMesh(Mesh* mesh);
void EndRenderPass();

// @font
struct FontGlyph
{
    vec2 uv_min;
    vec2 uv_max;
    vec2 size;
    float advance;
    vec2 bearing;
    vec2 sdf_offset;
};

float GetBaseline(Font* font);
Material* GetMaterial(Font* font);
const FontGlyph* GetGlyph(Font* font, char ch);
float GetKerning(Font* font, char first, char second);