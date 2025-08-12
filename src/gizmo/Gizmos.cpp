/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/gizmo/Gizmos.h>
#include <noz/gizmo/IGizmo.h>

namespace noz::debug
{
    // Static member definitions - hardcoded UV coordinates for palette texture
    // Colors are every 8 pixels, starting at pixel 4,4 in the 1024x1024 texture
    // Red (0,127): (4, 1020) -> UV (0.0039, 0.996)
    // Green (1,127): (12, 1020) -> UV (0.0117, 0.996)
    // Blue (2,127): (20, 1020) -> UV (0.0195, 0.996)
    glm::vec2 Gizmos::s_redColorUV = glm::vec2(4.0f / 1024.0f, 1020.0f / 1024.0f);
    glm::vec2 Gizmos::s_greenColorUV = glm::vec2(12.0f / 1024.0f, 1020.0f / 1024.0f);
    glm::vec2 Gizmos::s_blueColorUV = glm::vec2(20.0f / 1024.0f, 1020.0f / 1024.0f);

	Gizmos::Gizmos()
	{
	}

	Gizmos::~Gizmos()
	{
	}

	void Gizmos::load()
	{
		ISingleton<Gizmos>::load();
		instance()->loadInternal();
	}

	void Gizmos::loadInternal()
	{
		_gizmoMaterial = Object::create<noz::renderer::Material>("shaders/gizmo");
		assert(_gizmoMaterial);

        _gizmoMaterial->setTexture(Asset::load<noz::renderer::Texture>("textures/palette"));
    }

	void Gizmos::unload()
	{
		if (instance())
		{
			instance()->unloadInternal();
		}
		ISingleton<Gizmos>::unload();
	}

    void Gizmos::unloadInternal()
    {
    }

    void Gizmos::registerGizmo(std::shared_ptr<IGizmo> gizmo)
    {
        if (!gizmo)
            return;

        // Add to frame gizmo list (duplicates are fine, will be cleared after render)
        _frameGizmos.emplace_back(gizmo);
    }


    void Gizmos::update()
    {
    }

    void Gizmos::render(noz::renderer::CommandBuffer* commandBuffer, const noz::node::Camera& camera)
    {
        assert(commandBuffer);
        assert(_gizmoMaterial);

        commandBuffer->beginOpaquePass(false); // false = don't clear depth buffer since we want gizmos on top
        commandBuffer->setViewProjection(camera.viewMatrix(), camera.projectionMatrix());
        commandBuffer->bind(_gizmoMaterial);

        for (const auto& weakGizmo : _frameGizmos)
        {
            auto gizmo = weakGizmo.lock();
            if (gizmo)
                gizmo->renderGizmo(commandBuffer);
        }

        commandBuffer->endOpaquePass();
        _frameGizmos.clear();
    }
}