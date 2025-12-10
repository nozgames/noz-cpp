//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct CameraImpl : Camera {
    Vec2 position_offset;
    float rotation;
    Vec4 extents;
    Mat3 view;
    Mat3 inv_view;
    Vec2Int last_screen_size;
    bool dirty;
    Vec2 shake_offset;
    Vec2 shake_intensity;
    Vec2 shake_noise;
    float shake_duration;
    float shake_elapsed;
    noz::Rect viewport;
};

static Vec2Int GetEffectiveSize(CameraImpl* impl) {
    if (impl->viewport.width > 0 && impl->viewport.height > 0)
        return Vec2Int{static_cast<i32>(impl->viewport.width), static_cast<i32>(impl->viewport.height)};
    return GetScreenSize();
}

bool IsDirty(CameraImpl* impl) {
    if (impl->dirty)
        return true;

    Vec2Int current_size = GetEffectiveSize(impl);
    return current_size.x != impl->last_screen_size.x || current_size.y != impl->last_screen_size.y;
}

static CameraImpl* UpdateIfDirty(Camera* camera) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    //if (IsDirty(impl))
        UpdateCamera(camera);

    return impl;
}

static void UpdateCameraShake(CameraImpl* impl) {
    if (impl->shake_duration <= 0.0f)
        return;

    impl->shake_elapsed += GetFrameTime();
    float t = impl->shake_elapsed / impl->shake_duration;
    if (t >= 1.0f) {
        impl->shake_duration = 0.0f;
        impl->shake_offset = VEC2_ZERO;
        return;
    }

    impl->shake_offset = Vec2{
            impl->shake_intensity.x * (PerlinNoise(Vec2{impl->shake_noise.x, t * 20.0f}) - 0.5f) * 2.0f,
            impl->shake_intensity.y * (PerlinNoise(Vec2{impl->shake_noise.y, t * 20.0f}) - 0.5f) * 2.0f
            } * (1.0f - t);

    impl->dirty = true;
}

void Shake(Camera* camera, const Vec2& intensity, float duration) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    impl->shake_duration = duration;
    impl->shake_elapsed = 0.0f;
    impl->shake_intensity = intensity;
    impl->shake_noise = Vec2{
        RandomFloat(0.0f, 100.0f),
        RandomFloat(0.0f, 100.0f)
    };
}

void UpdateCamera(Camera* camera) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);

    UpdateCameraShake(impl);

    // if (!IsDirty(impl))
    //     return;

    float left = impl->extents.x;
    float right = impl->extents.y;
    float bottom = impl->extents.z;
    float top = impl->extents.w;

    Vec2Int screen_size = GetEffectiveSize(impl);
    float screen_aspect = (float)screen_size.x / (float)screen_size.y;

    bool is_auto_width = (abs(left) >= F32_MAX || abs(right) >= F32_MAX);
    bool is_auto_height = (abs(bottom) >= F32_MAX || abs(top) >= F32_MAX);

    float width, height;
    if (is_auto_width && is_auto_height) {
        height = 2.0f;
        width = height * screen_aspect;
        left = -width * 0.5f;
        right = width * 0.5f;
        bottom = -height * 0.5f;
        top = height * 0.5f;
    } else if (is_auto_width) {
        height = top - bottom;
        width = abs(height) * screen_aspect;
        left = -width * 0.5f;
        right = width * 0.5f;
    } else if (is_auto_height) {
        width = right - left;
        height = width / screen_aspect;
        bottom = -height * 0.5f;
        top = height * 0.5f;
    } else {
        width = right - left;
        height = top - bottom;
    }

    float zoomX = 2.0f / abs(width);
    float zoomY = 2.0f / abs(height);
    if ((top - bottom) < 0) zoomY = -zoomY;

    Vec2 center;
    center.x = (left + right) * 0.5f;
    center.y = (bottom + top) * 0.5f;

    Vec2 final_position = center + impl->position_offset + impl->shake_offset;

    float c = cos(impl->rotation);
    float s = sin(impl->rotation);

    impl->view = Mat3{.m = {
        c * zoomX, -s * zoomY, 0,
        s * zoomX, c * zoomY, 0,
        -(c * final_position.x + s * final_position.y) * zoomX,
        -(-s * final_position.x + c * final_position.y) * zoomY, 1
    }};

    impl->inv_view = Inverse(impl->view);
    impl->last_screen_size = GetEffectiveSize(impl);
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

void SetSize(Camera* camera, const Vec2& size) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);

    float hw = abs(size.x) * 0.5f;
    float hh = abs(size.y) * 0.5f;

    if (size.x == 0 && size.y == 0)
        impl->extents = Vec4{F32_MAX, F32_MAX, F32_MAX, F32_MAX};
    else if (size.x == 0)
        impl->extents = Vec4{F32_MAX, F32_MAX, -hh, hh};
    else if (size.y == 0)
        impl->extents = Vec4{-hw, hw, F32_MAX, F32_MAX};
    else
        impl->extents = Vec4{-hw, hw, -hh, hh};

    impl->dirty = true;
}

void SetExtents(Camera* camera, float left, float right, float bottom, float top) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    impl->extents = Vec4{left, right, bottom, top};
    impl->dirty = true;
}

Vec2 ScreenToWorld(Camera* camera, const Vec2& screen_pos) {
    CameraImpl* impl = UpdateIfDirty(camera);

    // Convert screen position to NDC (Normalized Device Coordinates)
    // If viewport is set, coordinates are relative to the viewport
    Vec2Int effectiveSize = GetEffectiveSize(impl);
    Vec2 viewportSize = {(f32)effectiveSize.x, (f32)effectiveSize.y};

    // Adjust screen_pos to be relative to viewport if set
    Vec2 localPos = screen_pos;
    if (impl->viewport.width > 0 && impl->viewport.height > 0) {
        localPos.x -= impl->viewport.x;
        localPos.y -= impl->viewport.y;
    }

    Vec2 ndc;
    ndc.x = localPos.x / viewportSize.x * 2.0f - 1.0f;
    ndc.y = localPos.y / viewportSize.y * 2.0f - 1.0f;

    // Transform NDC to world space
    Vec3 worldPos = impl->inv_view * Vec3{ndc.x, ndc.y, 1.0f};

    // Return as Vec2 (assuming homogeneous coordinate is 1)
    return Vec2{worldPos.x / worldPos.z, worldPos.y / worldPos.z};
}

Vec2 WorldToScreen(Camera* camera, const Vec2& world_pos)
{
    CameraImpl* impl = UpdateIfDirty(camera);

    // Transform world position to NDC using the view matrix
    Vec3 ndc = impl->view * Vec3{world_pos.x, world_pos.y, 1.0f};

    // Homogeneous divide (if needed)
    ndc.x /= ndc.z;
    ndc.y /= ndc.z;

    // Convert NDC [-1, 1] to screen coordinates
    // If viewport is set, coordinates are relative to the viewport
    Vec2Int effectiveSize = GetEffectiveSize(impl);
    Vec2 viewportSize = {(f32)effectiveSize.x, (f32)effectiveSize.y};
    Vec2 screen;
    screen.x = (ndc.x + 1.0f) * 0.5f * viewportSize.x;
    screen.y = (ndc.y + 1.0f) * 0.5f * viewportSize.y;

    // Add viewport offset if set
    if (impl->viewport.width > 0 && impl->viewport.height > 0) {
        screen.x += impl->viewport.x;
        screen.y += impl->viewport.y;
    }

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
    Vec2Int effectiveSize = GetEffectiveSize(impl);
    float aspectRatio = (float)effectiveSize.x / (float)effectiveSize.y;
    
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
        width = abs(height) * aspectRatio;
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
    
    Vec2 center = Vec2{(left + right) * 0.5f, (bottom + top) * 0.5f} + impl->position_offset + impl->shake_offset;
    Vec2 a = {center.x - width * 0.5f, center.y - height * 0.5f};
    Vec2 b = {center.x + width * 0.5f, center.y + height * 0.5f};
    Bounds2 bounds { Min(a,b), Max(a,b) };
    return bounds;
}

const Vec2& GetPosition(Camera* camera)
{
    return static_cast<CameraImpl*>(camera)->position_offset;
}

void SetViewport(Camera* camera, const noz::Rect& viewport)
{
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    impl->viewport = viewport;
    impl->dirty = true;
}

const noz::Rect& GetViewport(Camera* camera)
{
    return static_cast<CameraImpl*>(camera)->viewport;
}

Camera* CreateCamera(Allocator* allocator)
{
    CameraImpl* impl = (CameraImpl*)Alloc(allocator, sizeof(CameraImpl));
    impl->extents = Vec4{-400, 400, -300, 300}; // Default 800x600 centered at origin
    impl->position_offset = VEC2_ZERO;
    impl->rotation = 0.0f;
    impl->dirty = true;
    impl->last_screen_size = {0, 0};
    impl->viewport = {}; // Default: use full screen (width/height = 0)
    return impl;
}

