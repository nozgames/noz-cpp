/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::node
{
	NOZ_DEFINE_TYPEID(Camera)

    // Static member definition
    Camera* Camera::_mainCamera = nullptr;

    Camera::Camera()
        : _viewMatrix(1.0f)
        , _projectionMatrix(1.0f)
        , _viewProjectionMatrix(1.0f)
        , _viewMatrixDirty(true)
        , _viewProjectionDirty(true)
        , _isOrthographic(false)
    {
        // Set default perspective projection
        setPerspective(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    }

    void Camera::setPerspective(float fov, float aspectRatio, float nearPlane, float farPlane)
    {
        _projectionMatrix = noz::math::perspective(noz::math::radians(fov), aspectRatio, nearPlane, farPlane);
        _isOrthographic = false;
        _viewProjectionDirty = true;
    }

    void Camera::setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
    {
        _projectionMatrix = noz::math::ortho(left, right, bottom, top, nearPlane, farPlane);
        _isOrthographic = true;
        _viewProjectionDirty = true;
    }

    void Camera::updateAspectRatio(float aspectRatio)
    {
        // Recalculate projection matrix with new aspect ratio
        // This assumes we're using perspective projection
        // For orthographic, we'd need to store the original parameters
        setPerspective(45.0f, aspectRatio, 0.1f, 1000.0f);
    }

    const mat4& Camera::viewMatrix() const
    {
        if (_viewMatrixDirty)
        {
            updateMatrices();
        }
        return _viewMatrix;
    }

    const mat4& Camera::viewProjection() const
    {
        if (_viewMatrixDirty || _viewProjectionDirty)
        {
            updateMatrices();
        }
        return _viewProjectionMatrix;
    }

    void Camera::forceMatrixUpdate()
    {
        updateMatrices();
    }

    std::array<vec3, 4> Camera::frustumCornersWorldSpace() const
    {
        // For top-down view, we want the corners at world Y=0, not at camera Y
        float worldY = 0.0f; // World ground level
        
        // Get the inverse of the view-projection matrix
        mat4 invVP = glm::inverse(_projectionMatrix * viewMatrix());
        
        // NDC corners at world Y level (top-down view)
        std::array<vec3, 4> ndc;
        ndc[0] = vec3(-1, 0, -1); // near left
        ndc[1] = vec3( 1, 0, -1); // near right
        ndc[2] = vec3( 1, 0,  1); // far right
        ndc[3] = vec3(-1, 0,  1); // far left

        std::array<vec3, 4> corners;
        for (int i = 0; i < 4; ++i)
        {
            // Transform NDC to world space
            vec4 worldPos = invVP * vec4(ndc[i], 1.0f);
            worldPos /= worldPos.w; // Perspective divide
            
            // Use world Y level for top-down view
            corners[i] = vec3(worldPos.x, worldY, worldPos.z);
            
            // Debug: Print the first corner to see what values we're getting
            if (i == 0)
            {
                std::cout << "Frustum corner 0: " << corners[i].x << ", " << corners[i].y << ", " << corners[i].z << std::endl;
            }
        }
        return corners;
    }

    void Camera::update()
    {
        Node3d::update();
        
        // Mark matrices as dirty when camera moves
        _viewMatrixDirty = true;
        _viewProjectionDirty = true;
        
        // Set this camera as the main camera
        setAsMainCamera();

		forceMatrixUpdate();
    }

    void Camera::start()
    {
        Node3d::start();
    }

    void Camera::onAttachedToParent()
    {
        Node3d::onAttachedToParent();
        _viewMatrixDirty = true;
        _viewProjectionDirty = true;
    }

    void Camera::onDetachedFromParent()
    {
        Node3d::onDetachedFromParent();
        _viewMatrixDirty = true;
        _viewProjectionDirty = true;
    }

    void Camera::setAsMainCamera()
    {
        _mainCamera = this;
    }

    vec3 Camera::screenToWorld(const vec2& screenPos) const
    {
        if (!_isOrthographic)
        {
            // TODO: Implement perspective projection screenToWorld
            return vec3(0.0f);
        }

        // Get screen dimensions from Application
        auto* app = noz::Application::instance();
        float screenWidth = static_cast<float>(app->screenWidth());
        float screenHeight = static_cast<float>(app->screenHeight());

        // Convert screen coordinates to NDC (-1 to 1)
        float ndcX = (2.0f * screenPos.x / screenWidth) - 1.0f;
        float ndcY = 1.0f - (2.0f * screenPos.y / screenHeight); // Flip Y for screen coordinates

        // Create NDC point at world Y=0 plane (z doesn't matter for orthographic)
        vec4 ndcPoint = vec4(ndcX, ndcY, 0.0f, 1.0f);

        // Get inverse view-projection matrix
        mat4 invVP = glm::inverse(viewProjection());

        // Transform NDC to world space
        vec4 worldPoint = invVP * ndcPoint;
        worldPoint /= worldPoint.w; // Perspective divide

        // For orthographic top-down view, we want the result at Y=0
        return vec3(worldPoint.x, 0.0f, worldPoint.z);
    }

    vec2 Camera::worldToScreen(const vec3& worldPos) const
    {
        if (!_isOrthographic)
        {
            // TODO: Implement perspective projection worldToScreen
            return vec2(0.0f);
        }

        // Transform world position to clip space
        vec4 clipPos = viewProjection() * vec4(worldPos, 1.0f);
        clipPos /= clipPos.w; // Perspective divide

        // Convert NDC to screen coordinates
        auto* app = noz::Application::instance();
        float screenWidth = static_cast<float>(app->screenWidth());
        float screenHeight = static_cast<float>(app->screenHeight());

        float screenX = (clipPos.x + 1.0f) * 0.5f * screenWidth;
        float screenY = (1.0f - clipPos.y) * 0.5f * screenHeight; // Flip Y for screen coordinates

        return vec2(screenX, screenY);
    }

    void Camera::updateMatrices() const
    {
        // Calculate view matrix from node's transform
        _viewMatrix = worldToLocal();
        _viewProjectionMatrix = _projectionMatrix * _viewMatrix;
        
        _viewMatrixDirty = false;
        _viewProjectionDirty = false;
    }
} 