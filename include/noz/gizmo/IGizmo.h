#pragma once

#include <noz/Node.h>

namespace noz::renderer
{
    class CommandBuffer;
}

namespace noz::debug
{
    /**
     * @brief Base class for gizmo nodes that can be rendered for debugging
     * Inherits from Node to integrate with the scene graph
     */
    class IGizmo : public noz::node::Node
    {
    public:

		NOZ_DECLARE_TYPEID(IGizmo, noz::node::Node)

        virtual ~IGizmo() = default;

        void update() override;

        virtual void renderGizmo(noz::renderer::CommandBuffer* commandBuffer) = 0;

	protected:

		IGizmo() {}
    };
}
