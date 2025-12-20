//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct CameraImpl : Camera {
    Vec2 position_offset;
    float rotation;
    Vec4 extents;
    Mat3 view;
    Mat3 inv_view;
    Vec2Int screen_size;
    Vec2 shake_offset;
    Vec2 shake_intensity;
    Vec2 shake_noise;
    float shake_duration;
    float shake_elapsed;
    noz::Rect viewport;
    Bounds2 bounds;
};

static Vec2Int GetEffectiveSize(CameraImpl* impl, const Vec2Int& screen_size) {
    if (impl->viewport.width > 0 && impl->viewport.height > 0)
        return Vec2Int{static_cast<i32>(impl->viewport.width), static_cast<i32>(impl->viewport.height)};
    return screen_size;
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

static void UpdateBounds(Camera* camera) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);

    float aspectRatio = static_cast<float>(impl->screen_size.x) / static_cast<float>(impl->screen_size.y);

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

    if (!auto_left && !auto_right) {
        width = right - left;
        if (!auto_bottom && !auto_top)
            height = top - bottom;
        else
            height = width / aspectRatio;
    } else if (!auto_bottom && !auto_top) {
        height = top - bottom;
        width = abs(height) * aspectRatio;
    } else {
        width = 2.0f;
        height = 2.0f / aspectRatio;
    }

    // Calculate final bounds
    if (auto_left && auto_right) {
        left = -width * 0.5f;
        right = width * 0.5f;
    } else if (auto_left) {
        left = right - width;
    } else if (auto_right) {
        right = left + width;
    }

    if (auto_bottom && auto_top) {
        bottom = -height * 0.5f;
        top = height * 0.5f;
    } else if (auto_bottom) {
        bottom = top - height;
    } else if (auto_top) {
        top = bottom + height;
    }

    Vec2 center = Vec2{(left + right) * 0.5f, (bottom + top) * 0.5f} + impl->position_offset + impl->shake_offset;
    Vec2 a = {center.x - width * 0.5f, center.y - height * 0.5f};
    Vec2 b = {center.x + width * 0.5f, center.y + height * 0.5f};
    impl->bounds = { Min(a,b), Max(a,b) };
}

void UpdateCamera(Camera* camera) {
    UpdateCamera(camera, GetScreenSize());
}

void UpdateCamera(Camera* camera, const Vec2Int& available_size) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);

    UpdateCameraShake(impl);

    float left = impl->extents.x;
    float right = impl->extents.y;
    float bottom = impl->extents.z;
    float top = impl->extents.w;

    if (impl->viewport.width > 0 && impl->viewport.height > 0)
        impl->screen_size = Vec2Int{
            static_cast<i32>(impl->viewport.width),
            static_cast<i32>(impl->viewport.height)};
    else
        impl->screen_size = available_size;

    impl->screen_size = GetEffectiveSize(impl, available_size);
    float screen_aspect = static_cast<float>(impl->screen_size.x) / static_cast<float>(impl->screen_size.y);

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

    UpdateBounds(camera);
}

void SetPosition(Camera* camera, const Vec2& position) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    impl->position_offset = position;
}

void SetRotation(Camera* camera, float rotation) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    impl->rotation = rotation;
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
}

void SetExtents(Camera* camera, float left, float right, float bottom, float top) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    impl->extents = Vec4{left, right, bottom, top};
}

Vec2 ScreenToWorld(Camera* camera, const Vec2& screen_pos) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    Vec2 viewportSize = {
        static_cast<f32>(impl->screen_size.x),
        static_cast<f32>(impl->screen_size.y)
    };

    Vec2 localPos = screen_pos;
    if (impl->viewport.width > 0 && impl->viewport.height > 0) {
        localPos.x -= impl->viewport.x;
        localPos.y -= impl->viewport.y;
    }

    Vec2 ndc;
    ndc.x = localPos.x / viewportSize.x * 2.0f - 1.0f;
    ndc.y = localPos.y / viewportSize.y * 2.0f - 1.0f;
    Vec3 worldPos = impl->inv_view * Vec3{ndc.x, ndc.y, 1.0f};
    return Vec2{worldPos.x / worldPos.z, worldPos.y / worldPos.z};
}

Vec2 WorldToScreen(Camera* camera, const Vec2& world_pos) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    Vec3 ndc = impl->view * Vec3{world_pos.x, world_pos.y, 1.0f};
    ndc.x /= ndc.z;
    ndc.y /= ndc.z;

    Vec2 viewportSize = {
        static_cast<f32>(impl->screen_size.x),
        static_cast<f32>(impl->screen_size.y)
    };
    Vec2 screen;
    screen.x = (ndc.x + 1.0f) * 0.5f * viewportSize.x;
    screen.y = (ndc.y + 1.0f) * 0.5f * viewportSize.y;

    if (impl->viewport.width > 0 && impl->viewport.height > 0) {
        screen.x += impl->viewport.x;
        screen.y += impl->viewport.y;
    }

    return screen;
}

const Vec2& GetPosition(Camera* camera) {
    return static_cast<CameraImpl*>(camera)->position_offset;
}

void SetViewport(Camera* camera, const noz::Rect& viewport) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    impl->viewport = viewport;
}

const noz::Rect& GetViewport(Camera* camera) {
    return static_cast<CameraImpl*>(camera)->viewport;
}

const Mat3& GetViewMatrix(Camera* camera) {
    CameraImpl* impl = static_cast<CameraImpl*>(camera);
    return impl->view;
}

Vec2Int GetScreenSize(Camera* camera) {
    return ToVec2Int(GetWorldSize(camera) * ToVec2(static_cast<CameraImpl*>(camera)->screen_size));
}

Bounds2 GetWorldBounds(Camera* camera) {
    CameraImpl* impl=static_cast<CameraImpl*>(camera);
    return impl->bounds;
}

Vec2 GetWorldSize(Camera* camera) {
    return GetSize(static_cast<CameraImpl*>(camera)->bounds);
}

Camera* CreateCamera(Allocator* allocator) {
    CameraImpl* impl = static_cast<CameraImpl*>(Alloc(allocator, sizeof(CameraImpl)));
    impl->extents = Vec4{-1, 1, -1, 1};
    impl->position_offset = VEC2_ZERO;
    impl->rotation = 0.0f;
    impl->screen_size = {0, 0};
    impl->viewport = {};
    return impl;
}

