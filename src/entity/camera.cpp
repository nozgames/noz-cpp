//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct CameraImpl : Camera
{
    Vec2 position;
    float rotation;
    Vec2 size;
    Mat3 view;
    Mat3 inv_view;
    Vec2Int last_screen_size;
    bool dirty;
};

static bool IsDirty(CameraImpl* impl)
{
    if (impl->dirty)
        return true;

    Vec2Int current_screen_size = GetScreenSize();
    return current_screen_size.x != impl->last_screen_size.x || current_screen_size.y != impl->last_screen_size.y;
}

static CameraImpl* UpdateIfDirty(Camera* camera)
{
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    if (IsDirty(impl))
        UpdateCamera(camera);

    return impl;
}

void UpdateCamera(Camera* camera)
{
    CameraImpl* impl = static_cast<CameraImpl*>(camera);

    if (!IsDirty(impl))
        return;

    float aspectRatio = GetScreenAspectRatio();
    float zoomX;
    float zoomY;

    // Fit both horizontal and vertical
    if (impl->size.x != 0 && impl->size.y != 0)
    {
        // Both dimensions specified - ensure both fit
        // Calculate zoom needed for each dimension
        float zoomFromWidth = 2.0f / abs(impl->size.x);
        float zoomFromHeight = 2.0f / abs(impl->size.y);

        // Use the smaller zoom (to fit both dimensions)
        float zoom = Min(zoomFromWidth, zoomFromHeight / aspectRatio);

        zoomX = zoom;
        zoomY = zoom * aspectRatio;

        // Handle flipping
        if (impl->size.x < 0)
            zoomX = -zoomX;
        if (impl->size.y < 0)
            zoomY = -zoomY;
    }
    // Horizontal fit
    else if (impl->size.x != 0)
    {
        zoomX = 2.0f / impl->size.x;
        zoomY = zoomX * aspectRatio;
    }
    // Vertical fit
    else if (impl->size.y != 0)
    {
        zoomY = 2.0f / impl->size.y;
        zoomX = zoomY / aspectRatio;
    }
    else
    {
        zoomX = 1.0f;
        zoomY = 1.0f;
    }

    float c = cos(impl->rotation);
    float s = sin(impl->rotation);

    impl->view = Mat3{
        .m = {
            c * zoomX, s * zoomX, -(c * impl->position.x + s * impl->position.y) * zoomX,
            -s * zoomY, c * zoomY, -(-s * impl->position.x + c * impl->position.y) * zoomY,
            0, 0, 1}};

    impl->inv_view = Inverse(impl->view);
    impl->last_screen_size = GetScreenSize();
    impl->dirty = false;
}

void SetPosition(Camera* camera, const Vec2& position)
{
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    impl->position = position;
    impl->dirty = true;
}

void SetRotation(Camera* camera, float rotation)
{
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    impl->rotation = rotation;
    impl->dirty = true;
}

void SetSize(Camera* camera, const Vec2& size)
{
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    impl->size = size;
    impl->dirty = true;
}

Vec2 ScreenToWorld(Camera* camera, const Vec2& screen_pos)
{
    CameraImpl* impl = UpdateIfDirty(camera);

    // Convert screen position to NDC (Normalized Device Coordinates)
    // Assuming screen_pos is in pixels [0, screenWidth] x [0, screenHeight]
    Vec2Int screen_size_int = GetScreenSize();
    Vec2 screenSize = {(f32)screen_size_int.x, (f32)screen_size_int.y};
    Vec2 ndc;
    ndc.x = screen_pos.x / screenSize.x * 2.0f - 1.0f;
    ndc.y = screen_pos.y / screenSize.y * 2.0f - 1.0f;

    // Transform NDC to world space
    Vec3 worldPos = impl->inv_view * Vec3{ndc.x, ndc.y, 1.0f};

    // Return as Vec2 (assuming homogeneous coordinate is 1)
    return Vec2(worldPos.x / worldPos.z, worldPos.y / worldPos.z);
}

Vec2 WorldToScreen(Camera* camera, const Vec2& world_pos)
{
    CameraImpl* impl = UpdateIfDirty(camera);

    // Transform world position to NDC using the view matrix
    Vec3 ndc = impl->view * Vec3(world_pos.x, world_pos.y, 1.0f);

    // Homogeneous divide (if needed)
    ndc.x /= ndc.z;
    ndc.y /= ndc.z;

    // Convert NDC [-1, 1] to screen coordinates [0, screenSize]
    Vec2Int screen_size_int = GetScreenSize();
    Vec2 screenSize = {(f32)screen_size_int.x, (f32)screen_size_int.y};
    Vec2 screen;
    screen.x = (ndc.x + 1.0f) * 0.5f * screenSize.x;
    screen.y = (ndc.y + 1.0f) * 0.5f * screenSize.y;
    return screen;
}

const Mat3& GetViewMatrix(Camera* camera)
{
    return UpdateIfDirty(camera)->view;
}

Camera* CreateCamera(Allocator* allocator)
{
    CameraImpl* impl = (CameraImpl*)Alloc(allocator, sizeof(CameraImpl));
    impl->size = {800, 600};
    impl->position = VEC2_ZERO;
    impl->rotation = 0.0f;
    impl->dirty = true;
    impl->last_screen_size = {0, 0};
    return impl;
}
