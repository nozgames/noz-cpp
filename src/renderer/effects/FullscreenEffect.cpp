/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/renderer/effects/FullscreenEffect.h>

namespace noz::renderer::effects
{
    NOZ_DEFINE_TYPEID(FullscreenEffect);

    FullscreenEffect::FullscreenEffect() 
    {
    }
    
    void FullscreenEffect::initialize(const std::shared_ptr<Material>& material)
    {        
        _material = material;
        createFullscreenQuad();
    }

    void FullscreenEffect::createFullscreenQuad() 
    {
        MeshBuilder builder;
        
        // Create a fullscreen quad (-1 to 1 in NDC space)
        builder.addVertex(glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f));
        builder.addVertex(glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f));
        builder.addVertex(glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f));
        builder.addVertex(glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f));
        
        // Indices for two triangles
        builder.addTriangle(0, 1, 2);
        builder.addTriangle(0, 2, 3);
        
        _mesh = builder.build("FullscreenQuad");
    }

    void FullscreenEffect::render(CommandBuffer* commandBuffer) 
    {
        assert(commandBuffer);
        assert(_material);
        
        if (_material->shader()->samplerCount() > 0)
            _material->setTexture(commandBuffer->opaqueTexture());

        commandBuffer->setCamera(glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f));
        commandBuffer->setTransform(glm::mat4(1.0f));
        commandBuffer->bind(_material);
        commandBuffer->drawMesh(_mesh);
    }
}
