/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::renderer
{
	class Mesh;
	class Shader;
	class Material;
	class Texture;
	class CommandBuffer;
}

namespace noz::node
{
    /**
     * @brief Node-based mesh renderer that extends Node
     * Handles all mesh rendering including static and animated meshes
     * Uses parent's transform for positioning
     */
    class MeshRenderer : public Node
    {
    public:

		NOZ_DECLARE_TYPEID(MeshRenderer, Node);

        virtual ~MeshRenderer() = default;

        // Resource management
        void setMesh(const std::shared_ptr<noz::renderer::Mesh>& mesh) { _mesh = mesh; }
        void setMaterial(const std::shared_ptr<noz::renderer::Material>& material) { _material = material; }
		void enableShadowCasting(bool enable) { _castShadow = enable; }
		void disableShadowCasting() { _castShadow = false; }
        
        const std::shared_ptr<noz::renderer::Mesh>& mesh() const;
        const std::shared_ptr<noz::renderer::Material>& material() const;
        const std::shared_ptr<noz::renderer::Texture>& texture() const;
        
        void render(noz::renderer::CommandBuffer* commandBuffer) override;
        
        bool isValid() const { return _mesh && _material; }

		bool shouldCastShadow() const { return _castShadow; }

        void update() override;
        void start() override;

		std::vector<glm::mat4>& boneTransforms() { return _boneTransforms; }

    protected:

		MeshRenderer();

        // Node attachment callbacks
        void onAttachedToParent() override;
        void onDetachedFromParent() override;

    private:

		friend class Animator;
	
		void initialize() {} // for create pattern

		std::vector<glm::mat4> _boneTransforms;
        std::shared_ptr<noz::renderer::Mesh> _mesh;
        std::shared_ptr<noz::renderer::Material> _material;
		bool _castShadow;
    };

    inline const std::shared_ptr<noz::renderer::Mesh>& MeshRenderer::mesh() const { return _mesh; }
    inline const std::shared_ptr<noz::renderer::Material>& MeshRenderer::material() const { return _material; }

} 