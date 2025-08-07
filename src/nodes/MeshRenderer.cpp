/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::node
{
	NOZ_DEFINE_TYPEID(MeshRenderer);

    MeshRenderer::MeshRenderer()
    {
    }

    void MeshRenderer::render(noz::renderer::CommandBuffer* commandBuffer)
    {
		assert(commandBuffer);

		if (!_castShadow && commandBuffer->isShadowPass())
			return;

        if (!isValid())
            return;

        // Get transform from parent Node3d (guaranteed to be Node3d by assert in start())
        auto parent3d = std::static_pointer_cast<Node3d>(parent());

        // Record commands to the command buffer
        commandBuffer->bind(_shader);
        commandBuffer->bind(_texture);
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