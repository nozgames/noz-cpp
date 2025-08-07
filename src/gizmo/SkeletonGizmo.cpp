/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/gizmo/SkeletonGizmo.h>
#include <noz/gizmo/Gizmos.h>

namespace noz::debug
{
	NOZ_DEFINE_TYPEID(SkeletonGizmo)

    SkeletonGizmo::SkeletonGizmo()
        : _showBoneHierarchy(true)
        , _showBonePositions(true)
        , _showBoneAxes(false)
    {
    }

    SkeletonGizmo::~SkeletonGizmo()
    {
    }

	void SkeletonGizmo::initialize()
	{
		// empty for create pattern
	}

    void SkeletonGizmo::onAttachedToParent()
    {
        IGizmo::onAttachedToParent();
        
		_parent = parent()->as<noz::node::Node3d>();
		assert(_parent.lock());

		_renderer = _parent.lock()->child<noz::node::MeshRenderer>();
		assert(_renderer.lock());

		_animator = _parent.lock()->child<noz::node::Animator>();
		assert(_animator.lock());

		createMesh();
    }

    void SkeletonGizmo::onDetachedFromParent()
    {
		_parent.reset();
		_animator.reset();
		_renderer.reset();
        
        IGizmo::onDetachedFromParent();
    }

    void SkeletonGizmo::renderGizmo(noz::renderer::CommandBuffer* commandBuffer)
    {
		assert(commandBuffer);
		
        if (!_mesh)
            return;

        commandBuffer->setTransform(_parent.lock()->localToWorld());
		commandBuffer->setBones(_renderer.lock()->boneTransforms());
        commandBuffer->drawMesh(_mesh);
    }

    void SkeletonGizmo::createMesh()
    {
		auto animator = _animator.lock();
		assert(animator);
        
        auto skeleton = animator->skeleton();

        // Create a single mesh containing all bone shapes and joints
        _mesh = std::make_shared<noz::renderer::Mesh>("skeleton_gizmo_mesh");
        
        // Use MeshBuilder to create the skeleton mesh
        noz::renderer::MeshBuilder builder;
        
        const auto& bones = skeleton->bones();
        
        // Create diamond-shaped bone shapes using stored bone direction and length
        for (const auto& bone : bones)
        {
			auto bonePos = (glm::vec3)(bone.localToWorld * glm::vec4(0, 0, 0, 1));
			auto boneDir = bone.direction;
			auto boneLength = 0.1f; // bone.length;
			auto widestPos = bonePos + boneDir * boneLength * 0.1f; // Part way down the bone
			auto boneRadius = boneLength * 0.1f;

            builder.addPyramid(widestPos, bonePos, boneRadius, bone.index);
            builder.addPyramid(widestPos, bonePos + boneDir * boneLength, boneRadius, bone.index);
        }
        
		_mesh = builder.toMesh("SkeletonGizmo");
		_mesh->upload(true);
    }
}