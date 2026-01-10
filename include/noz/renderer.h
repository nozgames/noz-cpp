//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

#include "rect.h"

namespace noz {
    constexpr int MAX_BONES = 64;
    constexpr u32 MAX_UNIFORM_BUFFER_SIZE = sizeof(Mat4) * 64;
    constexpr int MESH_MAX_FRAMES = 32;  // Max frames for animated meshes
    constexpr int MESH_MAX_VERTEX_WEIGHTS = 4;

    // @types
    struct Camera {};
    struct Texture : Asset {};
    struct Material : Asset {};
    struct Font : Asset {};
    struct Shader : Asset {};
    struct Skeleton : Asset {};
    struct MeshBuilder {};
    struct Atlas : Asset {};
    struct Animator;

    // @renderer_traits
    struct RendererTraits {
        int max_frame_commands;
        i32 vsync;
        int msaa_samples;  // 0=off, 2=2x, 4=4x MSAA
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

    Texture* CreateTexture(Allocator* allocator, void* data, size_t width, size_t height, TextureFormat format, const Name* name, TextureFilter filter = TEXTURE_FILTER_LINEAR);
    Texture* CreateTexture(Allocator* allocator, int width, int height, TextureFormat format, const Name* name, TextureFilter filter = TEXTURE_FILTER_LINEAR);
    void UpdateTexture(Texture* texture, void* data);  // Update entire texture with new data
    int GetBytesPerPixel(TextureFormat format);
    Vec2Int GetSize(Texture* texture);

    // Texture arrays (for atlas binding)
    Texture* CreateTextureArray(Allocator* allocator, Atlas** atlases, int atlas_count, const Name* name);
    void BindTextureArray(Texture* texture_array, int slot);
    bool IsTextureArray(Texture* texture);
    int GetLayerCount(Texture* texture);

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
        Vec2 position;
        float depth;
        float opacity = 1.0f;
        Vec2 uv;
        Vec2 normal;
        int bone_index = 0;
        int atlas_index = 0;
        Vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    };

    extern Mesh* CreateMesh(
        Allocator* allocator,
        u16 vertex_count,
        const MeshVertex* vertices,
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
    extern void UpdateMesh(Mesh* mesh, const MeshVertex* vertices, u16 vertex_count, const u16* indices, u16 index_count);
    extern Bounds2 ToBounds(const MeshVertex* vertices, int vertex_count);

    // Animation support (mesh can have multiple frames)
    extern int GetFrameCount(Mesh* mesh);
    extern int GetFrameRate(Mesh* mesh);
    extern float GetFrameWidthUV(Mesh* mesh);
    extern float GetDuration(Mesh* mesh);
    extern int GetFrameIndex(Mesh* mesh, float time, bool loop=false);
    extern float Update(Mesh* mesh, float current_time, float speed=1.0f, bool loop=true);
    extern void SetAnimationInfo(Mesh* mesh, int frame_count, int frame_rate, float frame_width_uv);

    // @mesh_builder
    extern MeshBuilder* CreateMeshBuilder(Allocator* allocator, u16 max_vertices, u16 max_indices);
    extern void UpdateMesh(MeshBuilder* builder, Mesh* mesh);
    extern void Clear(MeshBuilder* builder);
    extern const MeshVertex* GetVertices(MeshBuilder* builder);
    extern const Vec2* GetUvs(MeshBuilder* builder);
    extern const u8* GetBoneIndices(MeshBuilder* builder);
    extern const u16* GetIndices(MeshBuilder* builder);
    extern u16 GetVertexCount(MeshBuilder* builder);
    extern u16 GetIndexCount(MeshBuilder* builder);
    extern void AddIndex(MeshBuilder* builder, u16 index);
    extern void AddTriangle(MeshBuilder* builder, u16 a, u16 b, u16 c);
    extern void SetBaseVertex(MeshBuilder* builder, u16 base_vertex);
    extern void SetBaseVertex(MeshBuilder* builder);
    extern void AddRaw(
        MeshBuilder* builder,
        i16 vertex_count,
        const Vec3* positions,
        const Vec2* uv0,
        i16 index_count,
        const u16* indices);
    extern void AddQuad(
        MeshBuilder* builder,
        const Vec2& forward,
        const Vec2& right,
        f32 width,
        f32 height,
        const Vec2& color_uv);

    extern void AddVertex(MeshBuilder* builder, const MeshVertex& vertex);
    extern void AddVertex(MeshBuilder* builder, const Vec2& position, const Vec2& uv, float depth=0.0f);
    inline void AddVertex(MeshBuilder* builder, const Vec2& position, float depth) {
        AddVertex(builder, position, VEC2_ZERO, depth);
    }
    inline void AddVertex(MeshBuilder* builder, const Vec2& position) {
        AddVertex(builder, {position.x, position.y}, 0.0f);
    }
    extern void SetBone(MeshBuilder* builder, int bone_index);
    extern void AddCircle(MeshBuilder* builder, const Vec2& center, f32 radius, int segments, const Vec2& uv_color);
    extern void AddCircleStroke(MeshBuilder* builder, const Vec2& center, f32 radius, f32 thickness, int segments, const Vec2& uv_color);
    extern void AddArc(MeshBuilder* builder, const Vec2& center, f32 radius, f32 start, f32 end, int segments, const Vec2& uv_color);

    // @render_buffer
    extern void BindSkeleton(const Mat3* bones, int bone_count, int stride=0);
    extern void BindSkeleton(const Mat3* bind_poses, int bind_pose_stride, Mat3* bones, int bone_stride, int bone_count);
    extern void BindIdentitySkeleton();
    extern void BindDefaultTexture(int texture_index);
    extern void BindColor(Color color);
    extern void BindColor(Color color, Color emission);
    extern void BindColor(Color color, const Vec2Int& color_offset);
    extern void BindColor(Color color, const Vec2Int& color_offset, Color emission);
    extern void BindCamera(Camera* camera);
    extern void BindVertexUserData(const void* data, size_t size);
    extern void BindFragmentUserData(const void* data, size_t size);
    extern void BindDepth(float depth, float depth_scale=1.0f);
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
    extern void BindMaterial(Material* material);
    extern void BindShader(Shader* shader);
    extern void BindTexture(Texture* texture);
    extern void DrawMesh(Mesh* mesh);
    extern void DrawMesh(Mesh* mesh, const Mat3& transform, Animator& animator, int bone_index);
    extern void DrawMesh(Mesh* mesh, const Mat3& transform);
    extern void DrawMesh(Mesh* mesh, const Mat3& transform, int frame_index);  // frame_index ignored (UV animation)
    extern void DrawMesh(Mesh* mesh, const Mat3& transform, float time, bool loop=false);  // time/loop ignored (UV animation)
    extern void DrawMesh(Mesh* mesh, const Mat3& transform, Animator& animator, int bone_index, float time);

    // @clipping
    extern void BeginClip();      // Start writing clip mask to stencil
    extern void EndClipWrite();   // Switch from writing to testing stencil
    extern void EndClip();        // End clipping, restore previous stencil state

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
    extern Material* CreateMaterial(Allocator* allocator, Font* font);
    extern Material* CreateMaterial(Allocator* allocator, Font* font, float outline_size, Color outline_color);

    // @camera
    extern Camera* CreateCamera(Allocator* allocator);
    extern const Vec2& GetPosition(Camera* camera);
    extern void SetPosition(Camera* camera, const Vec2& position);
    extern void SetRotation(Camera* camera, float rotation);
    extern void SetSize(Camera* camera, const Vec2& size);
    inline void SetSize(Camera* camera, float width, float height) {
        SetSize(camera, Vec2{width, height});
    }
    extern void SetExtents(Camera* camera, float left, float right, float bottom, float top);
    extern void SetViewport(Camera* camera, const noz::Rect& viewport);
    extern const noz::Rect& GetViewport(Camera* camera);
    extern Vec2 ScreenToWorld(Camera* camera, const Vec2& screen_pos);
    extern Vec2 WorldToScreen(Camera* camera, const Vec2& world_pos);
    extern void Update(Camera* camera);
    extern void Update(Camera* camera, const Vec2Int& available_size);
    extern const Mat3& GetViewMatrix(Camera* camera);
    extern Bounds2 GetWorldBounds(Camera* camera);
    extern Vec2 GetWorldSize(Camera* camera);
    extern Vec2Int GetScreenSize(Camera* camera);
    extern void Shake(Camera* camera, const Vec2& intensity, float duration);
    extern void SetUpdateFunc(Camera* camera, Bounds2 (*update_func)(Camera*, const Vec2Int& available));

    struct Bone;

    // @skeleton
    extern Skeleton* CreateSkeleton(Allocator* allocator, Bone* bones, int bone_count, const Name* name=NAME_NONE);
    extern int GetBoneCount(Skeleton* skeleton);
    extern int GetBoneIndex(Skeleton* skeleton, const Name* name);
    extern const Mat3& GetLocalToWorld(Skeleton* skeleton, int bone_index);
    extern const Mat3& GetBindPose(Skeleton* skeleton, int bone_index);
    extern const Transform& GetBoneTransform(Skeleton* skeleton, int bone_index);
    extern const Bone& GetBone(Skeleton* skeleton, int bone_index);

    // @postprocess
    struct PostProcessParams {
        float desaturation;  // 0.0 = full color, 1.0 = grayscale
        float blur;          // 0.0 = no blur, 1.0 = max blur (not yet implemented)
    };

    extern void EnablePostProcess(bool enabled);
    extern bool IsPostProcessEnabled();
    extern void SetPostProcessMaterial(Material* material);
    extern Material* GetPostProcessMaterial();
    extern void SetPostProcessParams(const PostProcessParams& params);
    extern const PostProcessParams& GetPostProcessParams();
    extern void BeginPostProcessPass();
    extern void EndPostProcessPass();
    extern void DrawPostProcessQuad(Material* material);

    // @ui_composite
    extern void SetUICompositeMaterial(Material* material);
    extern Material* GetUICompositeMaterial();

    extern Mesh* GetMesh(const Name* name);
}

extern noz::Mesh** MESH;
extern noz::Font** FONT;
extern noz::Texture** TEXTURE;
extern noz::Atlas** ATLAS;
extern noz::Texture* ATLAS_ARRAY;  // Texture array containing all atlases
extern noz::Shader** SHADER;
extern noz::Skeleton** SKELETON;

extern int MESH_COUNT;
extern int FONT_COUNT;
extern int TEXTURE_COUNT;
extern int SHADER_COUNT;
extern int SKELETON_COUNT;
extern int ATLAS_COUNT;
