#pragma once

#include <noz/gizmo/IGizmo.h>
#include <noz/Node3d.h>
#include <memory>
#include <vector>

namespace noz::renderer
{
    class CommandBuffer;
    class Shader;
    class Mesh;
}

namespace noz::debug
{
    /**
     * @brief Gizmo node for visualizing transform of parent Node3d (X, Y, Z axes)
     * Renders red, green, and blue cylinders with cones on top
     */
    class TransformGizmo : public IGizmo
    {
    public:
        NOZ_DECLARE_TYPEID(TransformGizmo, IGizmo)

        virtual ~TransformGizmo();

        // Render the transform gizmo
        void renderGizmo(noz::renderer::CommandBuffer* commandBuffer) override;

    protected:
        // Parent hierarchy callbacks
        void onAttachedToParent() override;
        void onDetachedFromParent() override;

    private:

		TransformGizmo();

		void initialize();

        // Static mesh creation
        static std::shared_ptr<noz::renderer::Mesh> createSharedMesh();
        static std::shared_ptr<noz::renderer::Mesh> getSharedMesh();

        // Gizmo configuration constants
        static constexpr float Scale = 1.0f;
        static constexpr float CylinderRadius = 0.05f;
        static constexpr float ConeHeight = 0.2f;
        static constexpr float CylinderLength = 1.0f;
        static constexpr float ConeRadius = 0.1f;

        // Static shared mesh
        static std::shared_ptr<noz::renderer::Mesh> s_sharedMesh;

    protected:
        std::weak_ptr<noz::node::Node3d> _parentNode3d;
    };

} // namespace noz::debug
