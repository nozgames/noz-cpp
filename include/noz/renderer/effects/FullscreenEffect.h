/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/Node.h>

namespace noz::renderer
{
    class Texture;
    class Mesh;
    class Shader;
    class CommandBuffer;
}

namespace noz::renderer::effects
{
    class FullscreenEffect : public noz::node::Node 
    {
    public:

        NOZ_DECLARE_TYPEID(FullscreenEffect, Node);
        
        virtual ~FullscreenEffect() = default;
    
        void setInputTexture(std::shared_ptr<Texture> texture);
        std::shared_ptr<Texture> getInputTexture() const { return _inputTexture; }
    
        std::shared_ptr<Mesh> getMesh() const { return _mesh; }
    
        virtual void render(CommandBuffer* commandBuffer);
    
    protected:

        FullscreenEffect();
        
        void initialize();
        virtual void setupShader() = 0;
        void createFullscreenQuad();
    
        std::shared_ptr<Mesh> _mesh;
        std::shared_ptr<Shader> _shader;
        std::shared_ptr<Texture> _inputTexture;
    };
}