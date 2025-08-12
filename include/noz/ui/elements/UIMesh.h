/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/ui/elements/Element.h>
#include <glm/glm.hpp>
#include <memory>

namespace noz::renderer
{
    class Mesh;
    class Shader;
}

namespace noz::ui
{
    /**
     * @brief UIMesh node for rendering 3D meshes at UI coordinates
     * Allows rendering of 3D meshes within the UI system with customizable shaders
     */
    class UIMesh : public Element
    {
    public:

        NOZ_DECLARE_TYPEID(UIMesh, Element)
        
        virtual ~UIMesh() = default;

        // Mesh management
        void setMesh(const std::shared_ptr<noz::renderer::Mesh>& mesh);
        void setMesh(const std::string& meshPath);
        const std::shared_ptr<noz::renderer::Mesh>& mesh() const { return _mesh; }
        
        void setMaterial(const std::shared_ptr<noz::renderer::Material>& material);

        const std::shared_ptr<noz::renderer::Material>& material() const;
                
        // Transform settings for the mesh within UI space
        void setMeshScale(const glm::vec3& scale) { _meshScale = scale; markLayoutDirty(); }
        const glm::vec3& meshScale() const { return _meshScale; }
        
        void setMeshRotation(const glm::vec3& rotation) { _meshRotation = rotation; }
        const glm::vec3& meshRotation() const { return _meshRotation; }
        
        void setMeshOffset(const glm::vec3& offset) { _meshOffset = offset; }
        const glm::vec3& meshOffset() const { return _meshOffset; }

    protected:

        UIMesh();
        
        // Override measurement to account for mesh size
        vec2 measureContent(const vec2& availableSize) override;
        
        // Override rendering to draw mesh
        void renderElement(noz::renderer::CommandBuffer* commandBuffer) override;

    private:

        std::shared_ptr<noz::renderer::Mesh> _mesh;
        std::shared_ptr<noz::renderer::Material> _material;
        
        // Transform properties for the mesh
        glm::vec3 _meshScale;
        glm::vec3 _meshRotation; // Euler angles in radians
        glm::vec3 _meshOffset;
        
        void initialize();
        
        // Helper methods
        void renderMesh(noz::renderer::CommandBuffer* commandBuffer, const noz::Rect& rect);
        glm::mat4 calculateMeshTransform(const noz::Rect& bounds) const;
        
        friend class Object;
    };

    inline const std::shared_ptr<noz::renderer::Material>& UIMesh::material() const 
    {
        return _material;
    }
}
