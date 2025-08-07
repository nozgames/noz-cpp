/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::node
{
    /**
     * @brief Node-based camera that extends Node3d
     * Handles camera projection and view matrix calculations
     */
    class Camera : public Node3d
    {
    public:

		NOZ_DECLARE_TYPEID(Camera, Node3d)

        Camera();
        virtual ~Camera() = default;

        // Static main camera access
        static Camera* main() { return _mainCamera; }

        // Projection settings
        void setPerspective(float fov, float aspectRatio, float nearPlane, float farPlane);
        void setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
        void updateAspectRatio(float aspectRatio);
        
        // Matrix access
        const mat4& projectionMatrix() const { return _projectionMatrix; }
        const mat4& viewMatrix() const;
        const mat4& viewProjection() const;
        
        void forceMatrixUpdate();

        // Frustum corners for top-down view
        std::array<vec3, 4> frustumCornersWorldSpace() const;

        // Node lifecycle
        void update() override;
        void start() override;

    protected:
        // Node attachment callbacks
        void onAttachedToParent() override;
        void onDetachedFromParent() override;

    private:
        // Matrix update methods
        void updateMatrices() const;
        
        // Camera activation
        void setAsMainCamera();
        
        // Cached matrices
        mutable mat4 _viewMatrix;
        mutable mat4 _projectionMatrix;
        mutable mat4 _viewProjectionMatrix;
        mutable bool _viewMatrixDirty;
        mutable bool _viewProjectionDirty;
        
        // Static main camera
        static Camera* _mainCamera;
    };
} 