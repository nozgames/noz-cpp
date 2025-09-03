//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct CameraImpl : Camera
{
    Vec2 position;
    Vec2 view_size;
    Vec2Int last_screen_size;
    float rotation;
    bool dirty;
    RenderCamera render;
};

static bool IsDirty(CameraImpl* impl)
{
    if (impl->dirty)
        return true;

    Vec2Int current_screen_size = GetScreenSize();
    return current_screen_size.x != impl->last_screen_size.x ||
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

    // Calculate projection matrix based on which dimensions are specified
    float screen_aspect = GetScreenAspectRatio();
    float final_width;
    float final_height;
    
    if (impl->view_size.x > 0.0f && impl->view_size.y > 0.0f) {
        // SetSize mode - both width and height specified, fit both on screen
        float view_aspect = impl->view_size.x / impl->view_size.y;
        
        if (view_aspect > screen_aspect) {
            // View is wider than screen - fit width, adjust height
            final_width = impl->view_size.x;
            final_height = final_width / screen_aspect;
        } else {
            // View is taller than screen - fit height, adjust width
            final_height = impl->view_size.y;
            final_width = final_height * screen_aspect;
        }
    } else if (impl->view_size.y > 0.0f) {
        // SetHeight mode - height specified, auto-calculate width
        final_height = impl->view_size.y;
        final_width = final_height * screen_aspect;
    } else if (impl->view_size.x > 0.0f) {
        // SetWidth mode - width specified, auto-calculate height  
        final_width = impl->view_size.x;
        final_height = final_width / screen_aspect;
    } else {
        // Default fallback - use reasonable defaults
        final_height = 20.0f;
        final_width = final_height * screen_aspect;
    }
    
    impl->render.position = impl->position;
    impl->render.rotation = { Sin(impl->rotation), Cos(impl->rotation) };
    impl->render.size = { final_width * 0.5f, final_height * 0.5f };
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
    impl->view_size = size;
    impl->dirty = true;
}

Vec2 ScreenToWorld(Camera* camera, const Vec2& screen_pos)
{
    UpdateIfDirty(camera);

    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    Vec2 screen_center = GetScreenCenter();

    // Convert to view space with camera size scaling
    Vec2 view {
        (screen_pos.x - screen_center.x) * impl->render.size.x / screen_center.x,
        (screen_pos.y - screen_center.y) * impl->render.size.y / screen_center.y
    };

    // Apply inverse rotation
    const Vec2& cs = impl->render.rotation;
    return impl->render.position + Vec2{
        view.x * cs.y - view.y * cs.x,
        view.x * cs.x + view.y * cs.y
    };
}

Vec2 WorldToScreen(Camera* camera, const Vec2& world_pos)
{
    UpdateIfDirty(camera);

    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    Vec2 screen_center = GetScreenCenter();

    // Translate to camera-relative position
    Vec2 translated = world_pos - impl->render.position;

    // Apply camera rotation (forward rotation this time)
    Vec2 cs = impl->render.rotation;  // {sin, cos}
    Vec2 view {
        translated.x * cs.y + translated.y * cs.x,   // cos * x + sin * y
        -translated.x * cs.x + translated.y * cs.y   // -sin * x + cos * y
    };

    // Scale from world units to screen pixels
    Vec2 screen {
        view.x * screen_center.x / impl->render.size.x,
        view.y * screen_center.y / impl->render.size.y
    };

    // Convert to screen coordinates
    return {
        screen.x + screen_center.x,
        screen.y + screen_center.y  // No flip
    };
}

RenderCamera GetRenderCamera(Camera* camera)
{
    return UpdateIfDirty(camera)->render;
}

Camera* CreateCamera(Allocator* allocator)
{
    CameraImpl* impl = (CameraImpl*)Alloc(allocator, sizeof(CameraImpl));
    impl->view_size = { 800, 600 };
    impl->position = VEC2_ZERO;
    impl->rotation = 0.0f;
    impl->dirty = true;
    impl->last_screen_size = {0, 0};
    return impl;
}
