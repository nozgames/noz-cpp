//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct CameraImpl
{
    ENTITY_BASE;
    ivec2 view_size;
    mat4 projection;
    mat4 view_projection;
    bool orthographic;
};

static CameraImpl* Impl(Camera* c) { return (CameraImpl*)Cast(c, TYPE_CAMERA); }

Camera* CreateCamera(Allocator* allocator)
{
    CameraImpl* camera = Impl((Camera*)CreateEntity(allocator, sizeof(CameraImpl), TYPE_CAMERA));
    camera->view_size = { 800, 600 };
    return (Camera*)camera;
}

const mat4& GetProjection(Camera* camera)
{
    return Impl(camera)->projection;
}

const mat4& GetViewProjection(Camera* camera)
{
    auto impl = Impl(camera);
    impl->view_projection = GetProjection(camera) * inverse(GetLocalToWorld(camera));
    return Impl(camera)->view_projection;
}

void SetOrthographic(Camera* camera, float left, float right, float top, float bottom, float near, float far)
{
    auto impl = Impl(camera);
    impl->projection = glm::ortho(left, right, top, bottom, near, far);
    impl->orthographic = true;
}

void SetOrthographic(Camera* camera, float view_height, float near, float far)
{
    float half_height = view_height * 0.5f;
    float half_width = half_height * GetScreenAspectRatio();
    SetOrthographic(camera, -half_width, half_width, -half_height, half_height, near, far);
}

bool IsOrthographic(Camera* camera)
{
    return Impl(camera)->orthographic;
}

vec3 ScreenToWorld(Camera* camera, const vec2& screen_pos)
{
    auto impl = Impl(camera);
    if (!impl->orthographic)
    {
        // TODO: Implement perspective projection screenToWorld
        return vec3(0.0f);
    }

    // Get screen dimensions from Application
    auto screen_size = GetScreenSize();

    // Convert screen coordinates to NDC (-1 to 1)
    float ndc_x = (2.0f * screen_pos.x / screen_size.x) - 1.0f;
    float ndc_y = 1.0f - (2.0f * screen_pos.y / screen_size.y); // Flip Y for screen coordinates

    // Create NDC point at world Y=0 plane (z doesn't matter for orthographic)
    auto ndc_point = vec4(ndc_x, ndc_y, 0.0f, 1.0f);

    // Get inverse view-projection matrix
    auto inv_vp = inverse(GetViewProjection(camera));

    // Transform NDC to world space
    auto world_point = inv_vp * ndc_point;
    world_point /= world_point.w; // Perspective divide

    // For orthographic top-down view, we want the result at Y=0
    return vec3(world_point.x, 0.0f, world_point.z);
}

vec2 WorldToScreen(Camera* camera, const vec3& world_pos)
{
    auto impl = Impl(camera);
    if (!impl->orthographic)
    {
        // TODO: Implement perspective projection worldToScreen
        return vec2(0.0f);
    }

    // Transform world position to clip space
    vec4 clipPos = GetViewProjection(camera) * vec4(world_pos, 1.0f);
    clipPos /= clipPos.w; // Perspective divide

    // Convert NDC to screen coordinates
    auto screen_size = GetScreenSize();
    return {
        (clipPos.x + 1.0f) * 0.5f * screen_size.x,
        (1.0f - clipPos.y) * 0.5f * screen_size.y
    };
}