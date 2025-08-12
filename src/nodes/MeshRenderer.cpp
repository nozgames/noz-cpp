/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/renderer/Material.h>
#include <noz/renderer/Shader.h>

namespace noz::node
{
	NOZ_DEFINE_TYPEID(MeshRenderer);

    MeshRenderer::MeshRenderer()
        : _castShadow(true)
        , _material(nullptr)
        , _mesh(nullptr)
    {
    }

    void MeshRenderer::render(noz::renderer::CommandBuffer* commandBuffer)
    {
		assert(commandBuffer);

		if (!_castShadow && commandBuffer->isShadowPass())
			return;

        if (!isValid())
            return;

        auto parent3d = parent()->as<Node3d>();

        commandBuffer->bind(_material);
        commandBuffer->setTransform(parent3d->localToWorld());
        
        // Use bone transforms if available, otherwise use identity bone
        if (_boneTransforms.size() > 0)
        {
            commandBuffer->setBones(_boneTransforms);
        }
        else
        {
            static std::vector<glm::mat4> identityBone = { glm::identity<glm::mat4>() };
            commandBuffer->setBones(identityBone);
        }
        
        commandBuffer->drawMesh(_mesh);
    }

    void MeshRenderer::update()
    {
        Node::update();
    }

    void MeshRenderer::start()
    {
        Node::start();
        
        assert(parent());
        assert(std::dynamic_pointer_cast<Node3d>(parent()));
    }

    void MeshRenderer::onAttachedToParent()
    {
        Node::onAttachedToParent();
    }

    void MeshRenderer::onDetachedFromParent()
    {
        Node::onDetachedFromParent();
    }            
} 