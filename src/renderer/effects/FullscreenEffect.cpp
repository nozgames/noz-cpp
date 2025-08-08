#include <noz/renderer/effects/FullscreenEffect.h>
#include <noz/renderer/MeshBuilder.h>
#include <noz/renderer/CommandBuffer.h>
#include <noz/renderer/Mesh.h>
#include <noz/renderer/Shader.h>
#include <noz/renderer/Texture.h>

namespace noz::renderer::effects
{
    NOZ_DEFINE_TYPEID(FullscreenEffect);

    FullscreenEffect::FullscreenEffect() 
    {
    }
    
    void FullscreenEffect::initialize() 
    {
        createFullscreenQuad();
    }

    void FullscreenEffect::setInputTexture(std::shared_ptr<Texture> texture) 
    {
        _inputTexture = texture;
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
        if (!_mesh || !_shader || !_inputTexture) 
        {
            return;
        }
        
        // Set up for fullscreen rendering
        // Use identity matrices since we're rendering in NDC space
        commandBuffer->setCamera(glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f));
        commandBuffer->setTransform(glm::mat4(1.0f));
        
        // Bind shader and texture
        commandBuffer->bindShader(_shader);
        commandBuffer->bind(_inputTexture);
        
        // Draw the fullscreen quad
        commandBuffer->drawMesh(_mesh);
    }
}