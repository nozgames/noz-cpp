//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

extern void RenderMesh(Mesh* mesh);
extern void BindMaterialInternal(Material* material);
extern void BindTextureInternal(Texture* texture, i32 slot);
extern void UploadMesh(Mesh* mesh);

enum RenderCommandType {
    RENDER_COMMAND_TYPE_BIND_LIGHT,
    RENDER_COMMAND_TYPE_BIND_VERTEX_USER,
    RENDER_COMMAND_TYPE_BIND_FRAGMENT_USER,
    RENDER_COMMAND_TYPE_BIND_CAMERA,
    RENDER_COMMAND_TYPE_BIND_DEFAULT_TEXTURE,
    RENDER_COMMAND_TYPE_DRAW_MESH,
    RENDER_COMMAND_TYPE_BEGIN_PASS,
    RENDER_COMMAND_TYPE_END_PASS,
};

struct BindCameraData
{
    Camera* camera;
};

struct DrawMeshData {
    Mesh* mesh;
    Material* material;
    Mat3 transform;
    float depth;
    float depth_scale;
    Color color;
    Color emission;
    Vec2 color_uv_offset;
};

struct BeginPassData
{
    Color clear_color;
};

struct BindDefaultTextureData
{
    int index;
};

struct BindLightData
{
    Vec3 light_dir;
    Color diffuse_color;
    Color shadow_color;
};

struct BindUserData
{
    u8 data[MAX_UNIFORM_BUFFER_SIZE];
};

struct RenderCommand
{
    RenderCommandType type;
    union 
    {
        BindCameraData bind_camera;
        BindLightData bind_light;
        BindDefaultTextureData bind_default_texture;
        BeginPassData begin_pass;
        DrawMeshData draw_mesh;
        BindUserData bind_user_data;
    } data;
};

struct RenderBuffer {
    RenderCommand* commands;
    size_t command_count;
    size_t command_count_max;
    size_t transform_count_max;
    bool is_full;
    Material* current_material;
    Mat3 current_transform;
    float current_depth;
    float current_depth_scale;
    Color current_color;
    Vec2 current_color_uv_offset;
    Color current_emission;
};

static RenderBuffer* g_render_buffer = nullptr;

static void AddRenderCommand(RenderCommand* cmd)
{
    // don't add the command if we are full
    if (g_render_buffer->is_full)
        return;

    g_render_buffer->commands[g_render_buffer->command_count++] = *cmd;
    g_render_buffer->is_full = g_render_buffer->command_count == g_render_buffer->command_count_max;
}

void ClearRenderCommands()
{
    g_render_buffer->command_count = 0;
    g_render_buffer->is_full = false;
}

void BeginRenderPass(Color clear_color)
{
    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_BEGIN_PASS,
        .data = {
            .begin_pass = {
                .clear_color = clear_color}}};
    AddRenderCommand(&cmd);
}

void EndRenderPass()
{
    RenderCommand cmd = { .type = RENDER_COMMAND_TYPE_END_PASS };
    AddRenderCommand(&cmd);
}

void BindDefaultTexture(int texture_index)
{
    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_BIND_DEFAULT_TEXTURE,
        .data = {
            .bind_default_texture = {
                .index = texture_index}} };
    AddRenderCommand(&cmd);
}

void BindCamera(Camera* camera)
{
    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_BIND_CAMERA,
        .data = {
            .bind_camera = {
                .camera = camera
            }
        }
    };
    AddRenderCommand(&cmd);
}

void BindMaterial(Material* material)
{
    assert(material);
    g_render_buffer->current_material = material;
}

void BindDepth(float depth, float depth_scale) {
    g_render_buffer->current_depth = depth;
    g_render_buffer->current_depth_scale = depth_scale;
}

void BindTransform(const Vec3& position, float rotation, const Vec2& scale) {
    g_render_buffer->current_transform = TRS(XY(position), rotation, scale);
    g_render_buffer->current_depth = position.z;
    g_render_buffer->current_depth_scale = 1.0f;
}

void BindTransform(const Vec3& position, const Vec2& rotation, const Vec2& scale) {
    g_render_buffer->current_transform = TRS(XY(position), rotation, scale);
    g_render_buffer->current_depth = position.z;
    g_render_buffer->current_depth_scale = 1.0f;
}

void BindVertexUserData(const void* data, size_t size)
{
    assert(size <= MAX_UNIFORM_BUFFER_SIZE);

    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_BIND_VERTEX_USER,
    };
    memcpy(cmd.data.bind_user_data.data, data, size);
    AddRenderCommand(&cmd);
}

void BindFragmentUserData(const void* data, size_t size)
{
    assert(size <= MAX_UNIFORM_BUFFER_SIZE);

    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_BIND_FRAGMENT_USER,
    };
    memcpy(cmd.data.bind_user_data.data, data, size);
    AddRenderCommand(&cmd);
}

void BindLight(const Vec3& light_dir, const Color& diffuse_color, const Color& shadow_color)
{
    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_BIND_LIGHT,
        .data = { .bind_light = { .light_dir = light_dir, .diffuse_color = diffuse_color, .shadow_color = shadow_color } }
    };
    AddRenderCommand(&cmd);
}

void BindTransform(Transform& transform) {
    g_render_buffer->current_transform = GetLocalToWorld(transform);
}

void BindTransform(const Mat3& parent_transform, const Animator& animator, int bone_index) {
    g_render_buffer->current_transform = parent_transform * animator.bones[bone_index];
}

void BindTransform(const Mat3& transform) {
    g_render_buffer->current_transform = transform;
}

void BindColor(Color color) {
    g_render_buffer->current_color = ToLinear(color);
    g_render_buffer->current_color_uv_offset = VEC2_ZERO;
    g_render_buffer->current_emission = COLOR_TRANSPARENT;
}

void BindColor(Color color, const Vec2& color_uv_offset) {
    g_render_buffer->current_color = ToLinear(color);
    g_render_buffer->current_color_uv_offset = color_uv_offset;
    g_render_buffer->current_emission = COLOR_TRANSPARENT;
}

void BindColor(Color color, const Vec2& color_uv_offset, Color emission) {
    g_render_buffer->current_color = ToLinear(color);
    g_render_buffer->current_color_uv_offset = color_uv_offset;
    g_render_buffer->current_emission = emission;
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

void DrawMesh(AnimatedMesh* mesh, const Mat3& transform, float time) {
    DrawMesh(mesh, transform, GetFrameIndex(mesh, time));
}

void DrawMesh(AnimatedMesh* mesh, const Mat3& transform, int frame_index) {
    if (!mesh)
        return;

    frame_index = Clamp(frame_index, 0, GetFrameCount(mesh) - 1);
    DrawMesh(GetFrame(mesh, frame_index), transform);
}

void DrawMesh(Mesh* mesh) {
    assert(mesh);

    if (!IsUploaded(mesh))
        UploadMesh(mesh);

    assert(IsUploaded(mesh));

    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_DRAW_MESH,
        .data = {
            .draw_mesh = {
                .mesh = mesh,
                .material = g_render_buffer->current_material,
                .transform = g_render_buffer->current_transform,
                .depth = g_render_buffer->current_depth,
                .depth_scale = g_render_buffer->current_depth_scale,
                .color = g_render_buffer->current_color,
                .emission = g_render_buffer->current_emission,
                .color_uv_offset = g_render_buffer->current_color_uv_offset,
            }
        }
    };

    AddRenderCommand(&cmd);
}

void ExecuteRenderCommands()
{
    RenderCommand* commands = g_render_buffer->commands;
    size_t command_count = g_render_buffer->command_count;
    
    for (size_t command_index=0; command_index < command_count; ++command_index)
    {
        RenderCommand* command = commands + command_index;
        switch (command->type)
        {
        case RENDER_COMMAND_TYPE_BIND_VERTEX_USER:
            platform::BindVertexUserData(command->data.bind_user_data.data, MAX_UNIFORM_BUFFER_SIZE);
            break;

        case RENDER_COMMAND_TYPE_BIND_FRAGMENT_USER:
            platform::BindFragmentUserData(command->data.bind_user_data.data, MAX_UNIFORM_BUFFER_SIZE);
            break;

        case RENDER_COMMAND_TYPE_BIND_LIGHT:
            platform::BindLight(command->data.bind_light.light_dir, command->data.bind_light.diffuse_color, command->data.bind_light.shadow_color);
            break;

        case RENDER_COMMAND_TYPE_BIND_CAMERA:
            platform::BindCamera(GetViewMatrix(command->data.bind_camera.camera));
            break;

        case RENDER_COMMAND_TYPE_DRAW_MESH:
            platform::BindColor(command->data.draw_mesh.color, command->data.draw_mesh.color_uv_offset, command->data.draw_mesh.emission);
            platform::BindTransform(command->data.draw_mesh.transform, command->data.draw_mesh.depth, command->data.draw_mesh.depth_scale);
            BindMaterialInternal(command->data.draw_mesh.material);
            RenderMesh(command->data.draw_mesh.mesh);
            break;

        case RENDER_COMMAND_TYPE_BEGIN_PASS:
            platform::BeginRenderPass(command->data.begin_pass.clear_color);
            break;

        case RENDER_COMMAND_TYPE_BIND_DEFAULT_TEXTURE:
            BindTextureInternal(TEXTURE_WHITE, command->data.bind_default_texture.index);
            break;

        case RENDER_COMMAND_TYPE_END_PASS:
            platform::EndRenderPass();
            break;
        }
    }
}

void InitRenderBuffer(const RendererTraits* traits) {
    size_t commands_size = traits->max_frame_commands * sizeof(RenderCommand);
    size_t buffer_size = sizeof(RenderBuffer) + commands_size;
    
    g_render_buffer = (RenderBuffer*)malloc(buffer_size);
    if (!g_render_buffer) {
        ExitOutOfMemory();
        return;
    }        

    memset(g_render_buffer, 0, buffer_size);
    g_render_buffer->commands = (RenderCommand*)((char*)g_render_buffer + sizeof(RenderBuffer));
    g_render_buffer->command_count_max = traits->max_frame_commands;
}

void ShutdownRenderBuffer() {
    assert(g_render_buffer);
    free(g_render_buffer);
    g_render_buffer = nullptr;
}
