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
struct Skeleton : Asset {};
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
Texture* GetTexture(Material* material, size_t index=0);

// @mesh
struct Mesh : Asset { };

struct MeshVertex
{
    Vec2 position;
    Vec2 uv0;
    Vec3 normal;
};

extern Mesh* CreateMesh(
    Allocator* allocator,
    u16 vertex_count,
    const Vec2* positions,
    const Vec3* normals,
    const Vec2* uvs,
    u16 index_count,
    u16* indices,
    const Name* name,
    bool upload = true);
extern Mesh* CreateMesh(Allocator* allocator, MeshBuilder* builder, const Name* name, bool upload = true);
extern u16 GetVertexCount(Mesh* mesh);
extern u16 GetIndexCount(Mesh* mesh);
extern const MeshVertex* GetVertices(Mesh* mesh);
extern const u16* GetIndices(Mesh* mesh);
extern Bounds2 GetBounds(Mesh* mesh);
extern bool OverlapPoint(Mesh* mesh, const Vec2& overlap_point);

// @mesh_builder
MeshBuilder* CreateMeshBuilder(Allocator* allocator, u16 max_vertices, u16 max_indices);
void Clear(MeshBuilder* builder);
const Vec2* GetPositions(MeshBuilder* builder);
const Vec3* GetNormals(MeshBuilder* builder);
const Vec2* GetUvs(MeshBuilder* builder);
const u8* GetBoneIndices(MeshBuilder* builder);
const u16* GetIndices(MeshBuilder* builder);
u16 GetVertexCount(MeshBuilder* builder);
u16 GetIndexCount(MeshBuilder* builder);
void AddIndex(MeshBuilder* builder, u16 index);
void AddTriangle(MeshBuilder* builder, u16 a, u16 b, u16 c);
void AddTriangle(MeshBuilder* builder, const Vec3& a, const Vec3& b, const Vec3& c);
void AddRaw(
    MeshBuilder* builder,
    i16 vertex_count,
    const Vec2* positions,
    const Vec3* normals,
    const Vec2* uv0,
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
    const Vec3& normal);
void AddVertex(MeshBuilder* builder, const Vec2& position, const Vec3& normal, const Vec2& uv);
void AddVertex(MeshBuilder* builder, const Vec2& position);

// @render_buffer
void BindDefaultTexture(int texture_index);
void BindColor(Color color);
void BindCamera(Camera* camera);
void BindTransform(const Vec2& position, float rotation, const Vec2& scale);
void BindTransform(const Vec2& position, const Vec2& rotation, const Vec2& scale);
void BindTransform(const Mat3& transform);
void BindTransform(Transform& transform);
void BindLight(const Vec3& light_dir, const Color& diffuse_color, const Color& shadow_color);
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
float GetLineHeight(Font* font);
Material* GetMaterial(Font* font);
const FontGlyph* GetGlyph(Font* font, char ch);
float GetKerning(Font* font, char first, char second);

// @camera
Camera* CreateCamera(Allocator* allocator);
const Vec2& GetPosition(Camera* camera);
void SetPosition(Camera* camera, const Vec2& position);
void SetRotation(Camera* camera, float rotation);
void SetSize(Camera* camera, const Vec2& size);
void SetExtents(Camera* camera, float left, float right, float bottom, float top, bool auto_size=true);
Vec2 ScreenToWorld(Camera* camera, const Vec2& screen_pos);
Vec2 WorldToScreen(Camera* camera, const Vec2& world_pos);
void UpdateCamera(Camera* camera);
const Mat3& GetViewMatrix(Camera* camera);
Bounds2 GetBounds(Camera* camera);

// @skeleton
constexpr int MAX_BONES = 64;

extern int GetBoneCount(Skeleton* skeleton);
extern int GetBoneIndex(Skeleton* skeleton, const Name* name);
extern const Mat3& GetLocalToWorld(Skeleton* skeleton, int bone_index);
extern const Mat3& GetWorldToLocal(Skeleton* skeleton, int bone_index);

// @animation
constexpr int MAX_ANIMATION_FRAMES = 64;
constexpr float ANIMATION_FRAME_RATE = 24.0f;
constexpr float ANIMATION_FRAME_RATE_INV = 1.0f / ANIMATION_FRAME_RATE;

extern int GetBoneCount(Animation* animation);

// @animator
struct BoneTransform
{
    Vec2 position;
    Vec2 scale;
    float rotation;
};

struct Animator
{
    Skeleton* skeleton;
    Animation* animation;
    float time;
    float speed;
    int last_frame;
    bool loop;
    Mat3 bones[MAX_BONES];
};

extern void Init(Animator& animator, Skeleton* skeleton);
extern void Play(Animator& animator, Animation* animation, float speed=1.0f, bool loop=false);
extern void Stop(Animator& animator);
extern void Update(Animator& animator, float time_scale=1.0f);
extern bool IsPlaying(Animator& animator);
extern int GetFrame(Animator& animator);
extern float GetTime(Animator& animator);
extern float GetNormalizedTime(Animator& animator);