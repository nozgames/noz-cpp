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
        
        std::shared_ptr<Mesh> mesh() const;
        std::shared_ptr<Material> material() const;
    
        virtual void render(CommandBuffer* commandBuffer);
    
    protected:

        FullscreenEffect();
        
        void initialize(const std::shared_ptr<Material>& material);
        void createFullscreenQuad();
    
        std::shared_ptr<Mesh> _mesh;
        std::shared_ptr<Material> _material;
    };

    inline std::shared_ptr<Mesh> FullscreenEffect::mesh() const { return _mesh; }
    inline std::shared_ptr<Material> FullscreenEffect::material() const { return _material; }

}