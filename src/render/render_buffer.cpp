//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

SDL_GPURenderPass* BeginUIPassGPU();

// todo: we can build a single bone buffer to upload and just add bone_offset to the model buffer for each mesh

enum RenderCommandType
{
    command_type_bind_material,
    command_type_bind_light,
    command_type_bind_transform,
    command_type_bind_camera,
    command_type_bind_bones,
    command_type_bind_default_texture,
    command_type_bind_color,
    command_type_set_viewport,
    command_type_set_scissor,
    command_type_draw_mesh,
    command_type_begin_pass,
    command_type_begin_ui_pass,
    command_type_begin_shadow_pass,
    command_type_begin_gamma_pass,
    command_type_end_pass,
};

struct BindMaterialData
{
    Material* material;
};

struct BindTransformData
{
    Mat4 transform;
};

struct BindCameraData
{
    Mat4 view;
    Mat4 projection;
    Mat4 view_projection;
    Mat4 light_view_projection;
};

struct BindBonesData
{
    size_t count;
    size_t offset;
};

struct BindLightData
{
    Vec3 ambient_color;
    float ambient_intensity;
    Vec3 diffuse_color;
    float diffuse_intensity;
    Vec3 direction;
    float shadow_bias;
};

struct SetViewportData
{
    SDL_GPUViewport gpu_viewport;
};

struct SetScissorData
{
    SDL_Rect rect;
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
    bool clear;
    Color color;
    bool msaa;
    Texture* target;
};

struct BindDefaultTextureData
{
    int index;
};

struct RenderCommand
{
    RenderCommandType type;
    union 
    {
        BindMaterialData bind_material;
        BindTransformData bind_transform;
        BindCameraData bind_camera;
        BindLightData bind_light;
        BindBonesData bind_bones;
        BindColorData bind_color;
        BindDefaultTextureData bind_default_texture;
        SetViewportData set_viewport;
        SetScissorData set_scissor;
        BeginPassData begin_pass;
        DrawMeshData draw_mesh;
    } data;
};

struct RenderBuffer
{
    RenderCommand* commands;
    size_t command_count;
    Mat4* transforms;
    size_t transform_count;
    size_t command_count_max;
    size_t transform_count_max;
    bool is_shadow_pass;
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
    g_render_buffer->transform_count = 0;
    g_render_buffer->is_shadow_pass = false;
    g_render_buffer->is_full = false;
    
    // add identity transform by default for all meshes with no bones
    g_render_buffer->transforms[0] = MAT4_IDENTITY;
    g_render_buffer->transform_count = 1;
}

void BeginRenderPass(bool clear, Color clear_color, bool msaa, Texture* target)
{
    RenderCommand cmd = {
        .type = command_type_begin_pass,
        .data = {
            .begin_pass = {
                .clear = clear,
                .color = clear_color,
                .msaa = msaa,
                .target = target}}};
    AddRenderCommand(&cmd);
}

void BeginUIPass()
{
    RenderCommand cmd = { .type = command_type_begin_ui_pass };
    AddRenderCommand(&cmd);
}

void BeginShadowPass(Mat4 light_view, Mat4 light_projection)
{
    RenderCommand cmd = {.type = command_type_begin_shadow_pass};
    AddRenderCommand(&cmd);
}

void EndRenderPass()
{
    RenderCommand cmd = { .type = command_type_end_pass };
    AddRenderCommand(&cmd);
}

void BeginGammaPass()
{
    RenderCommand cmd = { .type = command_type_begin_gamma_pass };
    AddRenderCommand(&cmd);
}

void BindDefaultTexture(int texture_index)
{
    RenderCommand cmd = {
        .type = command_type_bind_default_texture,
        .data = {
            .bind_default_texture = {
                .index = texture_index}} };
    AddRenderCommand(&cmd);
}

void BindCamera(const Mat4& view, const Mat4& projection)
{
    Mat4 view_projection = projection * view;
    RenderCommand cmd = {
        .type = command_type_bind_camera,
        .data = {
            .bind_camera = {
                .view=view,
                .projection=projection,
                .view_projection=view_projection,
                .light_view_projection=view_projection}}};
    AddRenderCommand(&cmd);
}

void BindMaterial(Material* material)
{
    assert(material);

    RenderCommand cmd = {
        .type = command_type_bind_material,
        .data = {
            .bind_material = {
                .material = material}} };

    AddRenderCommand(&cmd);
}

void BindTransform(const Mat3& transform)
{
    RenderCommand cmd = {
        .type = command_type_bind_transform,
        .data = {
            .bind_transform = {
                .transform = (Mat4)transform}} };
    AddRenderCommand(&cmd);
}

void BindTransform(const Mat4& transform)
{
    RenderCommand cmd = {
        .type = command_type_bind_transform,
        .data = {
            .bind_transform = {
                .transform = transform}} };
    AddRenderCommand(&cmd);
}

void BindBoneTransforms(const Mat4* bones, size_t bone_count)
{
    if (bone_count == 0)
        return;

    if (g_render_buffer->command_count + bone_count > g_render_buffer->transform_count_max)
    {
        g_render_buffer->is_full = true;
        return;
    }   

    RenderCommand cmd = {
        .type = command_type_bind_bones,
        .data = {
            .bind_bones = {
                .count = bone_count,
                .offset = g_render_buffer->transform_count}} };

    memcpy(
        g_render_buffer->transforms + g_render_buffer->transform_count,
        bones,
        bone_count * sizeof(Mat4));
    g_render_buffer->transform_count += bone_count;

    AddRenderCommand(&cmd);
}

void BindColor(Color color)
{
    RenderCommand cmd = {
        .type = command_type_bind_color,
        .data = {
            .bind_color = {
                .color = {color.r, color.g, color.b, color.a}}} };
    AddRenderCommand(&cmd);
}

void DrawMesh(Mesh* mesh)
{
    assert(mesh);
    RenderCommand cmd = {
        .type = command_type_draw_mesh,
        .data = {
            .draw_mesh = {
                .mesh = mesh}} };
    AddRenderCommand(&cmd);
}

void ExecuteRenderCommands(SDL_GPUCommandBuffer* cb)
{
    SDL_GPURenderPass* pass = nullptr;

    RenderCommand* commands = g_render_buffer->commands;
    size_t command_count = g_render_buffer->command_count;
    for (size_t command_index=0; command_index < command_count; ++command_index)
    {
        RenderCommand* command = commands + command_index;
        switch (command->type)
        {
        case command_type_bind_material:
            BindMaterialGPU(command->data.bind_material.material, cb);
            break;

        case command_type_bind_transform:
            SDL_PushGPUVertexUniformData(cb, vertex_register_object, &command->data, sizeof(BindTransformData));
            break;

        case command_type_bind_camera:
        {
            SDL_PushGPUVertexUniformData(cb, vertex_register_camera, &command->data, sizeof(BindCameraData));

            // Store for legacy compatibility (still needed by bindTransform for ObjectBuffer)
            //_view = data.view;
            //_view_projection = data.view_projection;

            // if (_shadowPassActive)
            //     _lightViewProjectionMatrix = _view_projection;

            break;
        }

        case command_type_bind_bones:
            SDL_PushGPUVertexUniformData(
                cb,
                vertex_register_bone,
                g_render_buffer->transforms + command->data.bind_bones.offset,
                (Uint32)command->data.bind_bones.count);
            break;

        case command_type_bind_light:
            SDL_PushGPUFragmentUniformData(cb, fragment_register_light, &command->data, sizeof(BindLightData));
            break;

        case command_type_bind_color:
            SDL_PushGPUFragmentUniformData(cb, fragment_register_color, &command->data, sizeof(BindColorData));
            break;

        case command_type_draw_mesh:
            DrawMeshGPU(command->data.draw_mesh.mesh, pass);
            break;

        case command_type_begin_pass:
            pass = BeginPassGPU(
                command->data.begin_pass.clear,
                command->data.begin_pass.color,
                command->data.begin_pass.msaa,
                command->data.begin_pass.target);
            break;

        case command_type_bind_default_texture:
            BindDefaultTextureGPU(command->data.bind_default_texture.index);
            break;

        case command_type_begin_gamma_pass:
            pass = BeginGammaPassGPU();
            break;

        case command_type_end_pass:
            EndRenderPassGPU();
            pass = nullptr;
            break;

        case command_type_begin_ui_pass:
            pass = BeginUIPassGPU();
            break;

        case command_type_begin_shadow_pass:
            pass = BeginShadowPassGPU();
            break;

        case command_type_set_viewport:
        {
            SDL_SetGPUViewport(pass, &command->data.set_viewport.gpu_viewport);
            break;
        }

        case command_type_set_scissor:
            SDL_SetGPUScissor(pass, &command->data.set_scissor.rect);
            break;
        }
    }
}

#if 0




    void render_buffer::bind_light(
        const vec3& direction,
        float ambient_intensity,
        const vec3& ambient_color,
        float diffuse_intensity,
        const vec3& diffuse_color,
        float shadow_bias)
    {
        impl()->commands.emplace_back
        (
            command_type_bind_light,
            command::bind_light
            {
                ambient_color,
                ambient_intensity,
                diffuse_color,
                diffuse_intensity,
                direction,
                shadow_bias
            });
    }





    void render_buffer::set_viewport(int x, int y, int width, int height)
    {
        impl()->commands.emplace_back(command_type_set_viewport, command::set_viewport { x, y, width, height });
    }

    void render_buffer::set_scissor(int x, int y, int width, int height)
    {
        impl()->commands.emplace_back(command_type_set_scissor, command::set_scissor { x, y, width, height });
    }

#endif

void InitRenderBuffer(RendererTraits* traits)
{
    size_t commands_size = traits->max_frame_commands * sizeof(RenderCommand);
    size_t transforms_size = traits->max_frame_transforms * sizeof(Mat4);
    size_t buffer_size = sizeof(RenderBuffer) + commands_size + transforms_size;
    
    g_render_buffer = (RenderBuffer*)malloc(buffer_size);
    if (!g_render_buffer)
    {
        ExitOutOfMemory();
        return;
    }        

    memset(g_render_buffer, 0, buffer_size);
    g_render_buffer->commands = (RenderCommand*)((char*)g_render_buffer + sizeof(RenderBuffer));
    g_render_buffer->transforms = (Mat4*)((char*)g_render_buffer->commands + commands_size);
    g_render_buffer->command_count_max = traits->max_frame_commands;
    g_render_buffer->transform_count_max = traits->max_frame_transforms;
}

void ShutdownRenderBuffer()
{
    assert(g_render_buffer);
    free(g_render_buffer);
    g_render_buffer = nullptr;
}
