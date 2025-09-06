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
    return
        current_screen_size.x != impl->last_screen_size.x ||
        current_screen_size.y != impl->last_screen_size.y;
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

    float c = cos(impl->rotation);
    float s = sin(impl->rotation);

    float aspectRatio = GetScreenAspectRatio();
    float zoom = 2.0f / impl->size.y;
    float zoomX = zoom / aspectRatio;
    float zoomY = zoom;

    impl->view = Mat3 { .m = {
        c * zoomX,  s * zoomX, -(c * impl->position.x + s * impl->position.y) * zoomX,
        -s * zoomY, c * zoomY, -(-s * impl->position.x + c * impl->position.y) * zoomY,
        0,          0,         1
    }};

    impl->inv_view = Inverse(impl->view);

    impl->last_screen_size = GetScreenSize();
    impl->dirty = false;

#if 0
    // Calculate projection matrix based on which dimensions are specified
    float screen_aspect = GetScreenAspectRatio();
    float final_width;
    float final_height;

    Vec2 view_size = impl->view_size;
    bool y_flip = view_size.y < 0.0f;
    if (y_flip)
        view_size.y *= -1.0f;
    y_flip = false;

    if (impl->view_size.x > 0.0f && impl->view_size.y > 0.0f)
    {
        // SetSize mode - both width and height specified, fit both on screen
        float view_aspect = impl->view_size.x / impl->view_size.y;

        if (view_aspect > screen_aspect)
        {
            // View is wider than screen - fit width, adjust height
            final_width = impl->view_size.x;
            final_height = final_width / screen_aspect;
        }
        else
        {
            // View is taller than screen - fit height, adjust width
            final_height = impl->view_size.y;
            final_width = final_height * screen_aspect;
        }
    }
    else if (impl->view_size.y > 0.0f)
    {
        // SetHeight mode - height specified, auto-calculate width
        final_height = impl->view_size.y;
        final_width = final_height * screen_aspect;
    }
    else if (impl->view_size.x > 0.0f)
    {
        // SetWidth mode - width specified, auto-calculate height
        final_width = impl->view_size.x;
        final_height = final_width / screen_aspect;
    }
    else
    {
        // Default fallback - use reasonable defaults
        final_height = 20.0f;
        final_width = final_height * screen_aspect;
    }

    impl->render.position = impl->position;
    impl->render.rotation = {Sin(impl->rotation), Cos(impl->rotation)};
    impl->render.size = {final_width * 0.5f, final_height * 0.5f * (y_flip ? -1.0f : 1.0f)};
    impl->last_screen_size = GetScreenSize();
    impl->dirty = false;
#endif

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
