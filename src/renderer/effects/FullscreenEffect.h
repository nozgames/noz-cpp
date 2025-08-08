#pragma once

#include "nodes/Node.h"
#include <memory>

namespace noz::renderer 
{
    class Mesh;
    class Shader;
    class Texture;
    class CommandBuffer;
}

namespace noz 
{
    class FullscreenEffect : public Node 
    {
    public:
        NOZ_DECLARE_TYPEID(FullscreenEffect, Node);
        
        virtual ~FullscreenEffect() = default;
    
        void setInputTexture(std::shared_ptr<renderer::Texture> texture);
        std::shared_ptr<renderer::Texture> getInputTexture() const { return _inputTexture; }
    
        virtual void render(renderer::CommandBuffer* commandBuffer);
    
    protected:
        FullscreenEffect();
        
        virtual void setupShader() = 0;
        void createFullscreenQuad();
    
        std::shared_ptr<renderer::Mesh> _mesh;
        std::shared_ptr<renderer::Shader> _shader;
        std::shared_ptr<renderer::Texture> _inputTexture;
        
    private:
        void initialize();
    };
}