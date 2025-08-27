//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct CameraImpl
{
    ENTITY_BASE;
    ivec2 view_size;
    mat4 projection;
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
