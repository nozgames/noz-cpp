    //
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../platform.h"

namespace noz {

    extern void RenderMesh(Mesh* mesh);
    extern void RenderMesh(Mesh* mesh, float time, bool loop = false);
    extern void BindMaterialInternal(Material* material);
    extern void BindTextureInternal(Texture* texture, i32 slot);
    extern void BindShaderInternal(Shader* shader);
    extern void UploadMesh(Mesh* mesh);

    enum RenderCommandType {
        RENDER_COMMAND_TYPE_BIND_VERTEX_USER,
        RENDER_COMMAND_TYPE_BIND_FRAGMENT_USER,
        RENDER_COMMAND_TYPE_BIND_CAMERA,
        RENDER_COMMAND_TYPE_BIND_DEFAULT_TEXTURE,
        RENDER_COMMAND_TYPE_BIND_SKELETON,
        RENDER_COMMAND_TYPE_DRAW_MESH,
        RENDER_COMMAND_TYPE_BEGIN_CLIP,
        RENDER_COMMAND_TYPE_END_CLIP_WRITE,
        RENDER_COMMAND_TYPE_END_CLIP
    };

    struct BindCameraData {
        noz::Rect viewport;
        Mat3 view_matrix;
    };

    struct DrawMeshData {
        Mesh* mesh;
        Material* material;
        Texture* texture;
        Shader* shader;
        Mat3 transform;
        float depth;
        float depth_scale;
        Color color;
        Color emission;
        Vec2Int color_offset;
    };

    struct BeginPassData {
        Color clear_color;
    };

    struct BindDefaultTextureData {
        int index;
    };

    struct BindUserData {
        u8 data[MAX_UNIFORM_BUFFER_SIZE];
    };

    // todo: change to current skeleton, no need for storing them in commands
    struct BindSkeletonData {
        int bone_count;
        Mat3 bones[MAX_BONES];
    };

    struct RenderCommand {
        RenderCommandType type;
        union {
            BindCameraData bind_camera;
            BindDefaultTextureData bind_default_texture;
            BeginPassData begin_pass;
            DrawMeshData draw_mesh;
            BindUserData bind_user_data;
            BindSkeletonData bind_skeleton;
        } data;
    };

    struct RenderBuffer {
        RenderCommand* commands;
        int command_count;
        int command_count_max;
        bool is_full;
        Material* current_material;
        Texture* current_texture;
        Shader* current_shader;
        Mat3 current_transform;
        float current_depth;
        float current_depth_scale;
        Color current_color;
        Vec2Int current_color_offset;
        Color current_emission;
    };

    static RenderBuffer g_render_buffer = {};

    static void AddRenderCommand(RenderCommand* cmd) {
        if (g_render_buffer.is_full) return;
        g_render_buffer.commands[g_render_buffer.command_count++] = *cmd;
        g_render_buffer.is_full = g_render_buffer.command_count == g_render_buffer.command_count_max;
    }

    void ClearRenderCommands() {
        g_render_buffer.command_count = 0;
        g_render_buffer.is_full = false;
    }

    void BindIdentitySkeleton() {
        RenderCommand cmd = { .type = RENDER_COMMAND_TYPE_BIND_SKELETON };
        cmd.data.bind_skeleton.bone_count = MAX_BONES;
        for (int i = 0; i < MAX_BONES; ++i)
            cmd.data.bind_skeleton.bones[i] = MAT3_IDENTITY;
        AddRenderCommand(&cmd);
    }

    void BindSkeleton(const Mat3* bind_poses, int bind_pose_stride, Mat3* bones, int bone_stride, int bone_count) {
        if (bone_stride == 0)
            bone_stride = sizeof(Mat3);

        if (bind_pose_stride == 0)
            bind_pose_stride = sizeof(Mat3);

        assert(bone_stride >= (int)sizeof(Mat3));
        assert(bind_pose_stride >= (int)sizeof(Mat3));

        const u8* bind_pose_bytes = (const u8*)bind_poses;
        const u8* bone_bytes = (const u8*)bones;

        RenderCommand cmd = { .type = RENDER_COMMAND_TYPE_BIND_SKELETON };
        cmd.data.bind_skeleton.bone_count = bone_count;
        for (int i = 0; i < bone_count; ++i, bind_pose_bytes += bind_pose_stride, bone_bytes += bone_stride) {
            const Mat3& bind_pose = *((Mat3*)bind_pose_bytes);
            const Mat3& bone = *((Mat3*)bone_bytes);
            Mat3 transform = bone * bind_pose;
            memcpy(&cmd.data.bind_skeleton.bones[i], &transform, sizeof(Mat3));
        }

        AddRenderCommand(&cmd);
    }

    void BindSkeleton(const Mat3* bones, int bone_count, int stride) {
        assert(stride == 0 || stride >= (int)sizeof(Mat3));

        RenderCommand cmd = { .type = RENDER_COMMAND_TYPE_BIND_SKELETON };
        cmd.data.bind_skeleton.bone_count = bone_count;
        if (stride == 0 || stride == sizeof(Mat3)) {
            memcpy(cmd.data.bind_skeleton.bones, bones, sizeof(Mat3) * bone_count);
        } else {
            for (int i = 0; i < bone_count; ++i) {
                memcpy(&cmd.data.bind_skeleton.bones[i], (const u8*)bones + i * stride, sizeof(Mat3));
            }
        }
        AddRenderCommand(&cmd);
    }

    void BindDefaultTexture(int texture_index) {
        RenderCommand cmd = {
            .type = RENDER_COMMAND_TYPE_BIND_DEFAULT_TEXTURE,
            .data = {
                .bind_default_texture = {
                    .index = texture_index}} };
        AddRenderCommand(&cmd);
    }

    void BindCamera(Camera* camera) {
        RenderCommand cmd = {
            .type = RENDER_COMMAND_TYPE_BIND_CAMERA,
            .data = {
                .bind_camera = {
                    .viewport = GetViewport(camera),
                    .view_matrix = GetViewMatrix(camera)
                }
            }
        };
        AddRenderCommand(&cmd);
    }

    void BindShader(Shader* shader) {
        g_render_buffer.current_shader = shader;
        g_render_buffer.current_material = nullptr;
    }

    void BindTexture(Texture* texture) {
        g_render_buffer.current_texture = texture;
        g_render_buffer.current_material = nullptr;
    }

    void BindMaterial(Material* material) {
        assert(material);
        g_render_buffer.current_material = material;
        g_render_buffer.current_texture = nullptr;
        g_render_buffer.current_shader = nullptr;
    }

    void BindDepth(float depth, float depth_scale) {
        g_render_buffer.current_depth = depth;
        g_render_buffer.current_depth_scale = depth_scale;
    }

    void BindTransform(const Vec3& position, float rotation, const Vec2& scale) {
        g_render_buffer.current_transform = TRS(XY(position), rotation, scale);
        g_render_buffer.current_depth = position.z;
        g_render_buffer.current_depth_scale = 1.0f;
    }

    void BindTransform(const Vec3& position, const Vec2& rotation, const Vec2& scale) {
        g_render_buffer.current_transform = TRS(XY(position), rotation, scale);
        g_render_buffer.current_depth = position.z;
        g_render_buffer.current_depth_scale = 1.0f;
    }

    void BindVertexUserData(const void* data, size_t size) {
        assert(size <= MAX_UNIFORM_BUFFER_SIZE);

        RenderCommand cmd = {
            .type = RENDER_COMMAND_TYPE_BIND_VERTEX_USER,
        };
        memcpy(cmd.data.bind_user_data.data, data, size);
        AddRenderCommand(&cmd);
    }

    void BindFragmentUserData(const void* data, size_t size) {
        assert(size <= MAX_UNIFORM_BUFFER_SIZE);

        RenderCommand cmd = {
            .type = RENDER_COMMAND_TYPE_BIND_FRAGMENT_USER,
        };
        memcpy(cmd.data.bind_user_data.data, data, size);
        AddRenderCommand(&cmd);
    }

    void BindTransform(Transform& transform) {
        g_render_buffer.current_transform = GetLocalToWorld(transform);
    }

    void BindTransform(const Mat3& parent_transform, const Animator& animator, int bone_index) {
        g_render_buffer.current_transform = parent_transform * animator.bones[bone_index];
    }

    void BindTransform(const Mat3& transform) {
        g_render_buffer.current_transform = transform;
    }

    void BindColor(Color color) {
        g_render_buffer.current_color = ToLinear(color);
        g_render_buffer.current_color_offset = VEC2INT_ZERO;
        g_render_buffer.current_emission = COLOR_TRANSPARENT;
    }

    void BindColor(Color color, Color emission) {
        g_render_buffer.current_color = ToLinear(color);
        g_render_buffer.current_color_offset = VEC2INT_ZERO;
        g_render_buffer.current_emission = emission;
    }

    void BindColor(Color color, const Vec2Int& color_offset) {
        g_render_buffer.current_color = ToLinear(color);
        g_render_buffer.current_color_offset = color_offset;
        g_render_buffer.current_emission = COLOR_TRANSPARENT;
    }

    void BindColor(Color color, const Vec2Int& color_uv_offset, Color emission) {
        g_render_buffer.current_color = ToLinear(color);
        g_render_buffer.current_color_offset = color_uv_offset;
        g_render_buffer.current_emission = ToLinear(emission);
    }

    // Clipping - deferred via command buffer
    void BeginClip() {
        RenderCommand cmd = { .type = RENDER_COMMAND_TYPE_BEGIN_CLIP };
        AddRenderCommand(&cmd);
    }

    void EndClipWrite() {
        RenderCommand cmd = { .type = RENDER_COMMAND_TYPE_END_CLIP_WRITE };
        AddRenderCommand(&cmd);
    }

    void EndClip() {
        RenderCommand cmd = { .type = RENDER_COMMAND_TYPE_END_CLIP };
        AddRenderCommand(&cmd);
    }

    void DrawMesh(Mesh* mesh, const Mat3& transform, Animator& animator, int bone_index) {
        BindTransform(transform, animator, bone_index);
        DrawMesh(mesh);
    }

    void DrawMesh(Mesh* mesh, const Mat3& transform) {
        if (!mesh)
            return;

        BindTransform(transform);
        DrawMesh(mesh);
    }

    void DrawMesh(Mesh* mesh, const Mat3& transform, float time, bool loop) {
        (void)time;
        (void)loop;
        BindTransform(transform);
        DrawMesh(mesh);
    }

    void DrawMesh(Mesh* mesh, const Mat3& transform, int frame_index) {
        (void)frame_index;
        BindTransform(transform);
        DrawMesh(mesh);
    }

    void DrawMesh(Mesh* mesh, const Mat3& transform, Animator& animator, int bone_index, float time) {
        (void)time;
        BindTransform(transform, animator, bone_index);
        DrawMesh(mesh);
    }

    void DrawMesh(Mesh* mesh) {
        if (!mesh) return;
        if (GetVertexCount(mesh) == 0 || GetIndexCount(mesh) == 0) return;

        if (!IsUploaded(mesh))
            UploadMesh(mesh);

        assert(IsUploaded(mesh));

        RenderCommand cmd = {
            .type = RENDER_COMMAND_TYPE_DRAW_MESH,
            .data = {
                .draw_mesh = {
                    .mesh = mesh,
                    .material = g_render_buffer.current_material,
                    .texture = g_render_buffer.current_texture,
                    .shader = g_render_buffer.current_shader,
                    .transform = g_render_buffer.current_transform,
                    .depth = g_render_buffer.current_depth,
                    .depth_scale = g_render_buffer.current_depth_scale,
                    .color = g_render_buffer.current_color,
                    .emission = g_render_buffer.current_emission,
                    .color_offset = g_render_buffer.current_color_offset,
                }
            }
        };

        AddRenderCommand(&cmd);
    }

    void ExecuteRenderCommands() {
        RenderCommand* commands = g_render_buffer.commands;
        int command_count = g_render_buffer.command_count;

        for (int command_index=0; command_index < command_count; ++command_index) {
            RenderCommand* command = commands + command_index;
            switch (command->type)
            {
                case RENDER_COMMAND_TYPE_BIND_VERTEX_USER:
                    PlatformBindVertexUserData(command->data.bind_user_data.data, MAX_UNIFORM_BUFFER_SIZE);
                    break;

                case RENDER_COMMAND_TYPE_BIND_FRAGMENT_USER:
                    PlatformBindFragmentUserData(command->data.bind_user_data.data, MAX_UNIFORM_BUFFER_SIZE);
                    break;

                case RENDER_COMMAND_TYPE_BIND_CAMERA:
                    PlatformSetViewport(command->data.bind_camera.viewport);
                    PlatformBindCamera(command->data.bind_camera.view_matrix);
                    break;

                case RENDER_COMMAND_TYPE_DRAW_MESH:
                    PlatformBindColor(
                        command->data.draw_mesh.color,
                        ToVec2(command->data.draw_mesh.color_offset),
                        command->data.draw_mesh.emission);
                    PlatformBindTransform(
                        command->data.draw_mesh.transform,
                        command->data.draw_mesh.depth,
                        command->data.draw_mesh.depth_scale);
                    if (command->data.draw_mesh.material)
                        BindMaterialInternal(command->data.draw_mesh.material);
                    if (command->data.draw_mesh.shader)
                        BindShaderInternal(command->data.draw_mesh.shader);
                    // Only bind loose texture if no material (material handles its own textures)
                    if (command->data.draw_mesh.texture && !command->data.draw_mesh.material)
                        BindTextureInternal(command->data.draw_mesh.texture, 0);

                    RenderMesh(command->data.draw_mesh.mesh);
                    break;

                case RENDER_COMMAND_TYPE_BIND_DEFAULT_TEXTURE:
                    BindTextureInternal(TEXTURE_WHITE, command->data.bind_default_texture.index);
                    break;

                case RENDER_COMMAND_TYPE_BIND_SKELETON:
                    PlatformBindSkeleton(command->data.bind_skeleton.bones, (u8)command->data.bind_skeleton.bone_count);
                    break;

                case RENDER_COMMAND_TYPE_BEGIN_CLIP:
                    PlatformBeginClip();
                    break;

                case RENDER_COMMAND_TYPE_END_CLIP_WRITE:
                    PlatformEndClipWrite();
                    break;

                case RENDER_COMMAND_TYPE_END_CLIP:
                    PlatformEndClip();
                    break;
            }
        }

        ClearRenderCommands();
    }

    void InitRenderBuffer(const RendererTraits* traits) {
        g_render_buffer.command_count_max = traits->max_frame_commands;
        g_render_buffer.commands = static_cast<RenderCommand*>(Alloc(ALLOCATOR_DEFAULT, traits->max_frame_commands * sizeof(RenderCommand)));
    }

    void ShutdownRenderBuffer() {
        Free(g_render_buffer.commands);
        g_render_buffer = {};
    }
}
