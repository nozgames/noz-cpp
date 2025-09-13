//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

extern void RenderMesh(Mesh* mesh);
extern void BindMaterialInternal(Material* material);
extern void BindTextureInternal(Texture* texture, i32 slot);

enum RenderCommandType
{
    RENDER_COMMAND_TYPE_BIND_MATERIAL,
    RENDER_COMMAND_TYPE_BIND_LIGHT,
    RENDER_COMMAND_TYPE_BIND_TRANSFORM,
    RENDER_COMMAND_TYPE_BIND_CAMERA,
    RENDER_COMMAND_TYPE_BIND_BONES,
    RENDER_COMMAND_TYPE_BIND_DEFAULT_TEXTURE,
    RENDER_COMMAND_TYPE_BIND_COLOR,
    RENDER_COMMAND_TYPE_DRAW_MESH,
    RENDER_COMMAND_TYPE_BEGIN_PASS,
    RENDER_COMMAND_TYPE_END_PASS,
};

struct BindMaterialData
{
    Material* material;
};

struct BindTransformData
{
    Mat3 transform;
};

struct BindCameraData
{
    Camera* camera;
};

struct BindBonesData
{
    size_t count;
    size_t offset;
};

struct BindColorData
{
    Color color;
};

struct DrawMeshData
{
    Mesh* mesh;
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

struct RenderCommand
{
    RenderCommandType type;
    union 
    {
        BindMaterialData bind_material;
        BindTransformData bind_transform;
        BindCameraData bind_camera;
        BindBonesData bind_bones;
        BindColorData bind_color;
        BindLightData bind_light;
        BindDefaultTextureData bind_default_texture;
        BeginPassData begin_pass;
        DrawMeshData draw_mesh;
    } data;
};

struct RenderBuffer
{
    RenderCommand* commands;
    size_t command_count;
    size_t command_count_max;
    size_t transform_count_max;
    bool is_full;
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

    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_BIND_MATERIAL,
        .data = {
            .bind_material = {
                .material = material}} };

    AddRenderCommand(&cmd);
}

void BindTransform(const Vec2& position, float rotation, const Vec2& scale)
{
    Mat3 trs = TRS(position, rotation, scale);

    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_BIND_TRANSFORM,
        .data = { .bind_transform = { .transform = trs }}
    };
    AddRenderCommand(&cmd);
}

void BindTransform(const Vec2& position, const Vec2& rotation, const Vec2& scale)
{
    Mat3 trs = TRS(position, rotation, scale);

    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_BIND_TRANSFORM,
        .data = { .bind_transform = { .transform = trs }}
    };
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

void BindTransform(Transform& transform)
{
    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_BIND_TRANSFORM,
        .data = { .bind_transform = { .transform = GetLocalToWorld(transform) }}
    };
    AddRenderCommand(&cmd);
}

void BindTransform(const Mat3& transform)
{
    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_BIND_TRANSFORM,
        .data = { .bind_transform = { .transform = transform }}
    };
    AddRenderCommand(&cmd);
}

void BindColor(Color color)
{
    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_BIND_COLOR,
        .data = {
            .bind_color = {
                .color = {color.r, color.g, color.b, color.a}}} };
    AddRenderCommand(&cmd);
}

void DrawMesh(Mesh* mesh)
{
    assert(mesh);
    RenderCommand cmd = {
        .type = RENDER_COMMAND_TYPE_DRAW_MESH,
        .data = {
            .draw_mesh = {
                .mesh = mesh}} };
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
        case RENDER_COMMAND_TYPE_BIND_MATERIAL:
            BindMaterialInternal(command->data.bind_material.material);
            break;

        case RENDER_COMMAND_TYPE_BIND_TRANSFORM:
            platform::BindTransform(command->data.bind_transform.transform);
            break;

        case RENDER_COMMAND_TYPE_BIND_LIGHT:
            platform::BindLight(command->data.bind_light.light_dir, command->data.bind_light.diffuse_color, command->data.bind_light.shadow_color);
            break;

        case RENDER_COMMAND_TYPE_BIND_CAMERA:
            platform::BindCamera(GetViewMatrix(command->data.bind_camera.camera));
            break;

        case RENDER_COMMAND_TYPE_BIND_COLOR:
            platform::BindColor(command->data.bind_color.color);
            break;

        case RENDER_COMMAND_TYPE_DRAW_MESH:
            RenderMesh(command->data.draw_mesh.mesh);
            break;

        case RENDER_COMMAND_TYPE_BEGIN_PASS:
            platform::BeginRenderPass(command->data.begin_pass.clear_color);
            break;

        case RENDER_COMMAND_TYPE_BIND_DEFAULT_TEXTURE:
            BindTextureInternal(g_core_assets.textures.white, command->data.bind_default_texture.index);
            break;

        case RENDER_COMMAND_TYPE_END_PASS:
            platform::EndRenderPass();
            break;
        }
    }
}

void InitRenderBuffer(const RendererTraits* traits)
{
    size_t commands_size = traits->max_frame_commands * sizeof(RenderCommand);
    size_t buffer_size = sizeof(RenderBuffer) + commands_size;
    
    g_render_buffer = (RenderBuffer*)malloc(buffer_size);
    if (!g_render_buffer)
    {
        ExitOutOfMemory();
        return;
    }        

    memset(g_render_buffer, 0, buffer_size);
    g_render_buffer->commands = (RenderCommand*)((char*)g_render_buffer + sizeof(RenderBuffer));
    g_render_buffer->command_count_max = traits->max_frame_commands;
}

void ShutdownRenderBuffer()
{
    assert(g_render_buffer);
    free(g_render_buffer);
    g_render_buffer = nullptr;
}
