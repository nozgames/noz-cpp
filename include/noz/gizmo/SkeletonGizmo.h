/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/gizmo/IGizmo.h>
#include <noz/nodes/Animator.h>

namespace noz::renderer
{
    class CommandBuffer;
    class Shader;
	class Skeleton;
	class Mesh;
}

namespace noz::debug
{
    /**
     * @brief Gizmo node for visualizing skeleton bones from a sibling Animator
     * Automatically finds and renders the skeleton of a sibling Animator node
     */
    class SkeletonGizmo : public IGizmo
    {
    public:
        NOZ_DECLARE_TYPEID(SkeletonGizmo, IGizmo)

        ~SkeletonGizmo();

        void renderGizmo(noz::renderer::CommandBuffer* commandBuffer) override;

    protected:
        // Parent hierarchy callbacks
        void onAttachedToParent() override;
        void onDetachedFromParent() override;

        void setShowBoneHierarchy(bool show) { _showBoneHierarchy = show; }
        void setShowBonePositions(bool show) { _showBonePositions = show; }
        void setShowBoneAxes(bool show) { _showBoneAxes = show; }

        bool showBoneHierarchy() const { return _showBoneHierarchy; }
        bool showBonePositions() const { return _showBonePositions; }
        bool showBoneAxes() const { return _showBoneAxes; }

    private:

		SkeletonGizmo();

		void initialize();

        void createMesh();

        bool _showBoneHierarchy;
        bool _showBonePositions;
        bool _showBoneAxes;

        std::weak_ptr<noz::node::Animator> _animator;
		std::weak_ptr<noz::node::Node3d> _parent;
		std::weak_ptr<noz::node::MeshRenderer> _renderer;
        std::shared_ptr<noz::renderer::Mesh> _mesh;
    };
}