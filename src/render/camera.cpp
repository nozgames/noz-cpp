//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct CameraImpl : Camera
{
    Vec2 position_offset; // Offset from the center of calculated extents
    float rotation;
    Vec4 extents; // left, right, bottom, top - F32_MIN/F32_MAX means auto-calculate
    Mat3 view;
    Mat3 inv_view;
    Vec2Int last_screen_size;
    bool dirty;
    bool auto_size_extents;
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

    if (impl->auto_size_extents)
    {
        float aspectRatio = GetScreenAspectRatio();

        // Calculate actual extents from smart extents
        float left = impl->extents.x;
        float right = impl->extents.y;
        float bottom = impl->extents.z;
        float top = impl->extents.w;

        // Handle auto-calculation for each extent
        bool auto_left = abs(left) >= F32_MAX;
        bool auto_right = abs(right) >= F32_MAX;
        bool auto_bottom = abs(bottom) >= F32_MAX;
        bool auto_top = abs(top) >= F32_MAX;

        // Determine if Y should be flipped
        bool flip_y = false;
        if ((auto_bottom && bottom < 0) || (auto_top && top < 0))
        {
            flip_y = true;
        }

        // Calculate width and height
        float width, height;

        if (!auto_left && !auto_right)
        {
            // Width is specified
            width = right - left;
            if (!auto_bottom && !auto_top)
            {
                // Both width and height specified
                height = top - bottom;
            }
            else
            {
                // Width specified, height auto-calculated from aspect ratio
                height = width / aspectRatio;
            }
        }
        else if (!auto_bottom && !auto_top)
        {
            // Height is specified, width auto-calculated
            height = top - bottom;
            width = height * aspectRatio;
        }
        else
        {
            // Both auto - use default
            width = 2.0f;
            height = 2.0f / aspectRatio;
        }

        // Now calculate final extent values
        if (auto_left && auto_right)
        {
            // Both horizontal extents auto - center around origin
            left = -width * 0.5f;
            right = width * 0.5f;
        }
        else if (auto_left)
        {
            left = right - width;
        }
        else if (auto_right)
        {
            right = left + width;
        }

        if (auto_bottom && auto_top)
        {
            // Both vertical extents auto - center around origin
            bottom = -height * 0.5f;
        }
        else if (auto_bottom)
        {
            bottom = top - height;
        }
        // Note: auto_top case doesn't need to calculate top since we only use bottom for positioning

        // Calculate zoom and position
        float zoomX = 2.0f / width;
        float zoomY = flip_y ? -2.0f / height : 2.0f / height;

        // Position camera so that the viewport covers the specified extents
        Vec2 center;
        center.x = (left + right) * 0.5f;
        center.y = bottom + height * 0.5f; // Camera Y = bottom edge + half height

        Vec2 final_position = center + impl->position_offset;

        float c = cos(impl->rotation);
        float s = sin(impl->rotation);

        impl->view = Mat3{.m = {c * zoomX, -s * zoomY, 0, s * zoomX, c * zoomY, 0,
                                -(c * final_position.x + s * final_position.y) * zoomX,
                                -(-s * final_position.x + c * final_position.y) * zoomY, 1}};
    }
    else
    {
        // Use the provided extents directly without any aspect ratio adjustments
        float left = impl->extents.x;
        float right = impl->extents.y;
        float bottom = impl->extents.z;
        float top = impl->extents.w;
        
        float width = right - left;
        float height = top - bottom;
        
        // Use uniform scaling to maintain square aspect ratio
        // Use the same zoom factor for both X and Y to prevent squishing
        float zoomX = 2.0f / width;
        float zoomY = 2.0f / height;
        
        // Position camera so that the viewport covers the specified extents exactly
        Vec2 center;
        center.x = (left + right) * 0.5f;
        center.y = (bottom + top) * 0.5f;
        
        Vec2 final_position = center + impl->position_offset;
        
        float c = cos(impl->rotation);
        float s = sin(impl->rotation);
        
        impl->view = Mat3{.m = {c * zoomX, -s * zoomY, 0, s * zoomX, c * zoomY, 0,
                                -(c * final_position.x + s * final_position.y) * zoomX,
                                -(-s * final_position.x + c * final_position.y) * zoomY, 1}};
    }

    impl->inv_view = Inverse(impl->view);
    impl->last_screen_size = GetScreenSize();
    impl->dirty = false;
}

void SetPosition(Camera* camera, const Vec2& position)
{
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    impl->position_offset = position;
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

    // Convert size to extents centered around current position
    float halfWidth = abs(size.x) * 0.5f;
    float halfHeight = abs(size.y) * 0.5f;

    if (size.x == 0 && size.y == 0)
    {
        // Both dimensions auto - use F32_MAX for all extents
        impl->extents = Vec4{F32_MAX, F32_MAX, F32_MAX, F32_MAX};
    }
    else if (size.x == 0)
    {
        // Width auto, height specified
        float sign_y = (size.y < 0) ? -1.0f : 1.0f;
        impl->extents = Vec4{F32_MAX, F32_MAX, -halfHeight * sign_y, halfHeight * sign_y};
    }
    else if (size.y == 0)
    {
        // Height auto, width specified
        float sign_x = (size.x < 0) ? -1.0f : 1.0f;
        impl->extents = Vec4{-halfWidth * sign_x, halfWidth * sign_x, F32_MAX, F32_MAX};
    }
    else
    {
        // Both dimensions specified
        float sign_x = (size.x < 0) ? -1.0f : 1.0f;
        float sign_y = (size.y < 0) ? -1.0f : 1.0f;
        impl->extents = Vec4{-halfWidth * sign_x, halfWidth * sign_x, -halfHeight * sign_y, halfHeight * sign_y};
    }

    impl->dirty = true;
}

void SetExtents(Camera* camera, float left, float right, float bottom, float top, bool auto_size)
{
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    impl->extents = Vec4{left, right, bottom, top};
    impl->dirty = true;
    impl->auto_size_extents = auto_size;
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

Bounds2 GetBounds(Camera* camera)
{
    CameraImpl* impl = UpdateIfDirty(camera);
    
    // Calculate the actual world bounds from the camera's current state
    float aspectRatio = GetScreenAspectRatio();
    
    // Get current extents
    float left = impl->extents.x;
    float right = impl->extents.y;
    float bottom = impl->extents.z;
    float top = impl->extents.w;
    
    // Handle auto-calculation for each extent (similar to UpdateCamera logic)
    bool auto_left = abs(left) >= F32_MAX;
    bool auto_right = abs(right) >= F32_MAX;
    bool auto_bottom = abs(bottom) >= F32_MAX;
    bool auto_top = abs(top) >= F32_MAX;
    
    // Calculate width and height
    float width, height;
    
    if (!auto_left && !auto_right)
    {
        width = right - left;
        if (!auto_bottom && !auto_top)
        {
            height = top - bottom;
        }
        else
        {
            height = width / aspectRatio;
        }
    }
    else if (!auto_bottom && !auto_top)
    {
        height = top - bottom;
        width = height * aspectRatio;
    }
    else
    {
        width = 2.0f;
        height = 2.0f / aspectRatio;
    }
    
    // Calculate final bounds
    if (auto_left && auto_right)
    {
        left = -width * 0.5f;
        right = width * 0.5f;
    }
    else if (auto_left)
    {
        left = right - width;
    }
    else if (auto_right)
    {
        right = left + width;
    }
    
    if (auto_bottom && auto_top)
    {
        bottom = -height * 0.5f;
        top = height * 0.5f;
    }
    else if (auto_bottom)
    {
        bottom = top - height;
    }
    else if (auto_top)
    {
        top = bottom + height;
    }
    
    Vec2 center = Vec2{(left + right) * 0.5f, (bottom + top) * 0.5f} + impl->position_offset;
    Vec2 a = {center.x - width * 0.5f, center.y - height * 0.5f};
    Vec2 b = {center.x + width * 0.5f, center.y + height * 0.5f};
    Bounds2 bounds { Min(a,b), Max(a,b) };
    return bounds;
}

const Vec2& GetPosition(Camera* camera)
{
    return static_cast<CameraImpl*>(camera)->position_offset;
}

Camera* CreateCamera(Allocator* allocator)
{
    CameraImpl* impl = (CameraImpl*)Alloc(allocator, sizeof(CameraImpl));
    impl->extents = Vec4{-400, 400, -300, 300}; // Default 800x600 centered at origin
    impl->position_offset = VEC2_ZERO;
    impl->rotation = 0.0f;
    impl->dirty = true;
    impl->last_screen_size = {0, 0};
    return impl;
}

