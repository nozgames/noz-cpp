//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

constexpr u32 MAX_UNIFORM_BUFFER_SIZE = sizeof(Mat4);

// @types
struct Camera {};
struct Texture : Asset {};
struct Material : Asset {};
struct Font : Asset {};
struct Shader : Asset {};
struct Animation : Asset {};
struct Skeleton : Asset {};
struct MeshBuilder {};
struct Animator;

// @renderer_traits
struct RendererTraits {
    size_t max_frame_commands;
    i32 vsync;
    bool msaa;
    float min_depth;
    float max_depth;
};

// @texture

enum TextureFilter {
    TEXTURE_FILTER_NEAREST,
    TEXTURE_FILTER_LINEAR
};

enum TextureClamp {
    TEXTURE_CLAMP_REPEAT,
    TEXTURE_CLAMP_CLAMP
};

enum TextureFormat {
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
void SetVertexData(Material* material, const void* data, size_t size);
void SetFragmentData(Material* material, const void* data, size_t size);

// @mesh
struct Mesh : Asset { };

struct MeshVertex {
    Vec3 position;
    Vec2 uv0;
    Vec2 padding;
};

extern Mesh* CreateMesh(
    Allocator* allocator,
    u16 vertex_count,
    const Vec3* positions,
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
extern Vec2 GetSize(Mesh* mesh);
extern bool OverlapPoint(Mesh* mesh, const Vec2& overlap_point);
extern bool IsUploaded(Mesh* mesh);

// @mesh_builder
extern MeshBuilder* CreateMeshBuilder(Allocator* allocator, u16 max_vertices, u16 max_indices);
extern void Clear(MeshBuilder* builder);
extern const Vec3* GetPositions(MeshBuilder* builder);
extern const Vec2* GetUvs(MeshBuilder* builder);
extern const u8* GetBoneIndices(MeshBuilder* builder);
extern const u16* GetIndices(MeshBuilder* builder);
extern u16 GetVertexCount(MeshBuilder* builder);
extern u16 GetIndexCount(MeshBuilder* builder);
extern void AddIndex(MeshBuilder* builder, u16 index);
extern void AddTriangle(MeshBuilder* builder, u16 a, u16 b, u16 c);
extern void AddTriangle(MeshBuilder* builder, const Vec3& a, const Vec3& b, const Vec3& c);
extern void AddRaw(
    MeshBuilder* builder,
    i16 vertex_count,
    const Vec3* positions,
    const Vec2* uv0,
    i16 index_count,
    const u16* indices);
extern void AddQuad(
    MeshBuilder* builder,
    const Vec3& forward,
    const Vec3& right,
    f32 width,
    f32 height,
    const Vec2& color_uv);
extern void AddQuad(
    MeshBuilder* builder,
    const Vec3& a,
    const Vec3& b,
    const Vec3& c,
    const Vec3& d,
    const Vec2& uv_color);
extern void AddVertex(MeshBuilder* builder, const Vec3& position, const Vec2& uv);
inline void AddVertex(MeshBuilder* builder, const Vec2& position, const Vec2& uv) {
    AddVertex(builder, {position.x, position.y, 0.0f}, uv);
}
extern void AddVertex(MeshBuilder* builder, const Vec3& position);
inline void AddVertex(MeshBuilder* builder, const Vec2& position) {
    AddVertex(builder, {position.x, position.y, 0.0f});
}
extern void AddCircle(MeshBuilder* builder, const Vec3& center, f32 radius, int segments, const Vec2& uv_color);
extern void AddCircleStroke(MeshBuilder* builder, const Vec3& center, f32 radius, f32 thickness, int segments, const Vec2& uv_color);
extern void AddArc(MeshBuilder* builder, const Vec3& center, f32 radius, f32 start, f32 end, int segments, const Vec2& uv_color);

// @render_buffer
extern void BindDefaultTexture(int texture_index);
extern void BindColor(Color color);
extern void BindColor(Color color, const Vec2& color_uv_offset);
extern void BindCamera(Camera* camera);
extern void BindVertexUserData(const void* data, size_t size);
extern void BindFragmentUserData(const void* data, size_t size);
extern void BindDepth(float depth);
extern void BindTransform(const Mat3& parent_transform, const Animator& animator, int bone_index);
extern void BindTransform(const Vec3& position, float rotation, const Vec2& scale);
inline void BindTransform(const Vec2& position, float rotation, const Vec2& scale) {
    BindTransform({position.x, position.y, 0.0f}, rotation, scale);
}
extern void BindTransform(const Vec3& position, const Vec2& rotation, const Vec2& scale);
inline void BindTransform(const Vec2& position, const Vec2& rotation, const Vec2& scale) {
    BindTransform({position.x, position.y, 0.0f}, rotation, scale);
}
extern void BindTransform(const Mat3& transform);
extern void BindTransform(Transform& transform);
extern void BindLight(const Vec3& light_dir, const Color& diffuse_color, const Color& shadow_color);
extern void BindMaterial(Material* material);
extern void DrawMesh(Mesh* mesh);
extern void DrawMesh(Mesh* mesh, const Mat3& transform, Animator& animator, int bone_index);
extern void DrawMesh(Mesh* mesh, const Mat3& transform);

// @font
struct FontGlyph {
    Vec2 uv_min;
    Vec2 uv_max;
    Vec2 size;
    float advance;
    Vec2 bearing;
};

extern int GetFontIndex(const Name* name);
extern float GetBaseline(Font* font);
extern float GetLineHeight(Font* font);
extern Material* GetMaterial(Font* font);
extern const FontGlyph* GetGlyph(Font* font, char ch);
extern float GetKerning(Font* font, char first, char second);

// @camera
extern Camera* CreateCamera(Allocator* allocator);
extern const Vec2& GetPosition(Camera* camera);
extern void SetPosition(Camera* camera, const Vec2& position);
extern void SetRotation(Camera* camera, float rotation);
extern void SetSize(Camera* camera, const Vec2& size);
extern void SetExtents(Camera* camera, float left, float right, float bottom, float top, bool auto_size=true);
extern Vec2 ScreenToWorld(Camera* camera, const Vec2& screen_pos);
extern Vec2 WorldToScreen(Camera* camera, const Vec2& world_pos);
extern void UpdateCamera(Camera* camera);
extern const Mat3& GetViewMatrix(Camera* camera);
extern Bounds2 GetBounds(Camera* camera);

// @skeleton
constexpr int MAX_BONES = 64;

extern int GetBoneCount(Skeleton* skeleton);
extern int GetBoneIndex(Skeleton* skeleton, const Name* name);
extern const Mat3& GetLocalToWorld(Skeleton* skeleton, int bone_index);
extern const Mat3& GetWorldToLocal(Skeleton* skeleton, int bone_index);

// @animation
constexpr int MAX_ANIMATION_FRAMES = 64;
constexpr int ANIMATION_FRAME_RATE = 12;

extern int GetBoneCount(Animation* animation);

// @animator
struct BoneTransform {
    Vec2 position;
    Vec2 scale;
    float rotation;
};

struct Animator {
    Skeleton* skeleton;
    Animation* animation;
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

extern Mesh** MESH;
extern Font** FONT;
extern Texture** TEXTURE;
extern Shader** SHADER;
extern Skeleton** SKELETON;
extern Animation** ANIMATION;