/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/ui/elements/UIMesh.h>
#include <noz/renderer/Mesh.h>
#include <noz/renderer/Shader.h>
#include <noz/renderer/CommandBuffer.h>
#include <noz/Asset.h>
#include <noz/Log.h>
#include <noz/math/bounds3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace noz::ui
{
    NOZ_DEFINE_TYPEID(UIMesh)

    UIMesh::UIMesh()
        : Element()
        , _mesh(nullptr)
        , _shader(nullptr)
        , _meshScale(1.0f, 1.0f, 1.0f)
        , _meshRotation(0.0f, 0.0f, 0.0f)
        , _meshOffset(0.0f, 0.0f, 0.0f)
    {
        setName("UIMesh");
    }
    
    void UIMesh::initialize()
    {
        // Base element initialization
    }

    void UIMesh::setMesh(const std::shared_ptr<noz::renderer::Mesh>& mesh)
    {
        if (_mesh != mesh)
        {
            _mesh = mesh;
            markLayoutDirty();
        }
    }

    void UIMesh::setMesh(const std::string& meshPath)
    {
        // Load the mesh using the resource system
        auto mesh = Asset::load<noz::renderer::Mesh>(meshPath);
        if (!mesh)
        {
            Log::error("UIMesh", "Failed to load mesh: " + meshPath);
            return;
        }
        
        setMesh(mesh);
    }
    
    void UIMesh::setShader(const std::shared_ptr<noz::renderer::Shader>& shader)
    {
        _shader = shader;
    }

    void UIMesh::setShader(const std::string& shaderPath)
    {
        // Load the shader using the resource system
        auto shader = Asset::load<noz::renderer::Shader>(shaderPath);
        if (!shader)
        {
            Log::error("UIMesh", "Failed to load shader: " + shaderPath);
            return;
        }
        
        setShader(shader);
    }

    vec2 UIMesh::measureContent(const vec2& availableSize)
    {
        if (!_mesh)
            return vec2(0.0f, 0.0f);
        
        // Use the mesh bounds scaled by our scale factor
        // For UI purposes, we'll use the 2D projection of the mesh bounds
        auto meshBounds = _mesh->bounds();
        
        float width = (meshBounds.max().x - meshBounds.min().x) * _meshScale.x;
        float height = (meshBounds.max().y - meshBounds.min().y) * _meshScale.y;
        
        // Ensure minimum size if mesh bounds are zero
        if (width <= 0.0f) width = 64.0f; // Default size
        if (height <= 0.0f) height = 64.0f;
        
        return vec2(width, height);
    }

    void UIMesh::renderElement(noz::renderer::CommandBuffer* commandBuffer)
    {
        // Render background first
        Element::renderElement(commandBuffer);
        
        // Render mesh if we have one
        if (_mesh && commandBuffer)
        {
            renderMesh(commandBuffer, bounds());
        }
    }

    void UIMesh::renderMesh(noz::renderer::CommandBuffer* commandBuffer, const noz::Rect& rect)
    {
        if (!_mesh)
            return;

        // Use custom shader if provided, otherwise fall back to a default 3D shader
        auto shaderToUse = _shader;
        if (!shaderToUse)
        {
            // Try to load a default mesh shader for UI use
            shaderToUse = Asset::load<noz::renderer::Shader>("shaders/ui_mesh");
            if (!shaderToUse)
            {
                // Fall back to the basic UI shader (may not look right for 3D meshes)
                shaderToUse = Asset::load<noz::renderer::Shader>("shaders/ui");
            }
        }
        
        if (!shaderToUse)
        {
            Log::error("UIMesh", "No shader available for mesh rendering");
            return;
        }

        // Calculate transform matrix for the mesh
        glm::mat4 transform = calculateMeshTransform(rect);

        // Record commands for drawing the mesh
        commandBuffer->bind(shaderToUse);
        commandBuffer->setTransform(transform);
        commandBuffer->setColor(glm::vec4(1.0f)); // Default white color
        commandBuffer->bindDefaultTexture(); // Default texture for now
        commandBuffer->drawMesh(_mesh);
    }

    glm::mat4 UIMesh::calculateMeshTransform(const noz::Rect& bounds) const
    {
        // Start with identity
        glm::mat4 transform = glm::mat4(1.0f);
        
        // Calculate center position within the UI bounds
        float centerX = bounds.x + bounds.width * 0.5f;
        float centerY = bounds.y + bounds.height * 0.5f;
        float centerZ = 0.0f; // Keep at UI depth for now

        // Apply translation to center of UI element bounds
        transform = glm::translate(transform, glm::vec3(centerX, centerY, centerZ));
        
        // Apply mesh offset
        transform = glm::translate(transform, _meshOffset);
        
        transform = transform *
            mat4_cast(quat(vec3(radians(_meshRotation.x), radians(_meshRotation.y), radians(_meshRotation.z))));

        // Apply scale
        transform = glm::scale(transform, _meshScale);
        
        // Scale to fit within the UI bounds if desired
        // For now, we'll let the user control this via meshScale
        
        return transform;
    }
}