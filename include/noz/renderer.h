//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// @types
struct Camera {};
struct Texture : Asset {};
struct Material : Asset {};
struct Font : Asset {};
struct Shader : Asset {};
struct Animation : Asset {};
struct MeshBuilder {};

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
    i32 vsync;
    bool msaa;
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
    TEXTURE_CLAMP_CLAMP
};

enum TextureFormat
{
    TEXTURE_FORMAT_RGBA8,
    TEXTURE_FORMAT_RGBA16F,
    TEXTURE_FORMAT_R8
};

Texture* CreateTexture(Allocator* allocator, void* data, size_t width, size_t height, TextureFormat format, const Name* name);
Texture* CreateTexture(Allocator* allocator, int width, int height, TextureFormat format, const Name* name);
int GetBytesPerPixel(TextureFormat format);
Vec2Int GetSize(Texture* texture);

// @material
Material* CreateMaterial(Allocator* allocator, Shader* shader);
Shader* GetShader(Material* material);
void SetTexture(Material* material, Texture* texture, size_t index=0);

// @mesh
struct Mesh : Asset { };

Mesh* CreateMesh(
    Allocator* allocator,
    u16 vertex_count,
    const Vec2* positions,
    const Vec2* normals,
    const Vec2* uvs,
    u8* bone_indices,
    u16 index_count,
    u16* indices,
    const Name* name);
Mesh* CreateMesh(Allocator* allocator, MeshBuilder* builder, const Name* name);

// @mesh_builder
MeshBuilder* CreateMeshBuilder(Allocator* allocator, int max_vertices, int max_indices);
void Clear(MeshBuilder* builder);
const Vec2* GetPositions(MeshBuilder* builder);
const Vec2* GetNormals(MeshBuilder* builder);
const Vec2* GetUvs(MeshBuilder* builder);
const u8* GetBoneIndices(MeshBuilder* builder);
const u16* GetIndices(MeshBuilder* builder);
u32 GetVertexCount(MeshBuilder* builder);
u32 GetIndexCount(MeshBuilder* builder);
void AddIndex(MeshBuilder* builder, uint16_t index);
void AddTriangle(MeshBuilder* builder, uint16_t a, uint16_t b, uint16_t c);
void AddTriangle(MeshBuilder* builder, const Vec3& a, const Vec3& b, const Vec3& c, uint8_t bone_index);
void AddPyramid(MeshBuilder* builder, const Vec3& start, const Vec3& end, float size, uint8_t bone_index);
void AddCube(MeshBuilder* builder, const Vec3& center, const Vec3& size, uint8_t bone_index);
void AddRaw(
    MeshBuilder* builder,
    i16 vertex_count,
    const Vec2* positions,
    const Vec2* normals,
    const Vec2* uv0,
    u8 bone_index,
    i16 index_count,
    const u16* indices);
void AddQuad(
    MeshBuilder* builder,
    const Vec2& forward,
    const Vec2& right,
    f32 width,
    f32 height,
    const Vec2& color_uv);
void AddQuad(
    MeshBuilder* builder,
    const Vec2& a,
    const Vec2& b,
    const Vec2& c,
    const Vec2& d,
    const Vec2& uv_color,
    const Vec2& normal,
    uint8_t bone_index=0);
void AddVertex(
    MeshBuilder* builder,
    const Vec2& position,
    const Vec2& normal,
    const Vec2& uv,
    uint8_t bone_index=0);

// @render_buffer
void BindDefaultTexture(int texture_index);
void BindColor(Color color);
void BindCamera(Camera* camera);
void BindTransform(const Vec2& position, float rotation, const Vec2& scale);
void BindMaterial(Material* material);
void DrawMesh(Mesh* mesh);

// @font
struct FontGlyph
{
    Vec2 uv_min;
    Vec2 uv_max;
    Vec2 size;
    float advance;
    Vec2 bearing;
};

float GetBaseline(Font* font);
Material* GetMaterial(Font* font);
const FontGlyph* GetGlyph(Font* font, char ch);
float GetKerning(Font* font, char first, char second);

// @camera
Camera* CreateCamera(Allocator* allocator);
void SetPosition(Camera* camera, const Vec2& position);
void SetRotation(Camera* camera, float rotation);
void SetSize(Camera* camera, const Vec2& size);
Vec2 ScreenToWorld(Camera* camera, const Vec2& screen_pos);
Vec2 WorldToScreen(Camera* camera, const Vec2& world_pos);
void UpdateCamera(Camera* camera);
