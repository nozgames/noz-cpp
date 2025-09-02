//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// @types
struct Camera;
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
    size_t vertex_count,
    Vec3* positions,
    Vec3* normals,
    Vec2* uvs,
    u8* bone_indices,
    size_t index_count,
    u16* indices,
    const Name* name);
Mesh* CreateMesh(Allocator* allocator, MeshBuilder* builder, const Name* name);

// @mesh_builder
MeshBuilder* CreateMeshBuilder(Allocator* allocator, int max_vertices, int max_indices);
void Clear(MeshBuilder* builder);
Vec3* GetPositions(MeshBuilder* builder);
Vec3* GetNormals(MeshBuilder* builder);
Vec2* GetUvs(MeshBuilder* builder);
u8* GetBoneIndices(MeshBuilder* builder);
u16* GetIndices(MeshBuilder* builder);
size_t GetVertexCount(MeshBuilder* builder);
size_t GetIndexCount(MeshBuilder* builder);
void AddIndex(MeshBuilder* builder, uint16_t index);
void AddTriangle(MeshBuilder* builder, uint16_t a, uint16_t b, uint16_t c);
void AddTriangle(MeshBuilder* builder, const Vec3& a, const Vec3& b, const Vec3& c, uint8_t bone_index);
void AddPyramid(MeshBuilder* builder, const Vec3& start, const Vec3& end, float size, uint8_t bone_index);
void AddCube(MeshBuilder* builder, const Vec3& center, const Vec3& size, uint8_t bone_index);
void AddRaw(
    MeshBuilder* builder,
    size_t vertex_count,
    const Vec3* positions,
    const Vec3* normals,
    const Vec2* uv0,
    u8 bone_index,
    size_t index_count,
    const uint16_t* indices);
void AddQuad(
    MeshBuilder* builder,
    const Vec3& forward,
    const Vec3& right,
    f32 width,
    f32 height,
    const Vec2& color_uv);
void AddQuad(
    MeshBuilder* builder,
    const Vec3& a,
    const Vec3& b,
    const Vec3& c,
    const Vec3& d,
    const Vec2& uv_color,
    const Vec3& normal,
    uint8_t bone_index=0);
void AddVertex(
    MeshBuilder* builder,
    const Vec3& position,
    const Vec3& normal,
    const Vec2& uv,
    uint8_t bone_index=0);

// @render_buffer
void BindDefaultTexture(int texture_index);
void BindColor(Color color);
void BindCamera(Camera* camera);
void BindCamera(const Mat4& view, const Mat4& projection);
void BindTransform(const Mat3& transform);
void BindTransform(const Mat4& transform);
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
    Vec2 sdf_offset;
};

float GetBaseline(Font* font);
Material* GetMaterial(Font* font);
const FontGlyph* GetGlyph(Font* font, char ch);
float GetKerning(Font* font, char first, char second);