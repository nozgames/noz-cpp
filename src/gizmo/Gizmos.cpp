#include <noz/gizmo/Gizmos.h>
#include <noz/gizmo/IGizmo.h>
#include <noz/nodes/Camera.h>
#include <noz/Resources.h>
#include <noz/renderer/CommandBuffer.h>
#include <noz/renderer/Shader.h>
#include <noz/renderer/Texture.h>

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
        // Load shared gizmo shader
        _gizmoShader = noz::Resources::instance()->load<noz::renderer::Shader>("shaders/gizmo");
		assert(_gizmoShader);

		_paletteTexture = noz::Resources::instance()->load<noz::renderer::Texture>("textures/palette");
		assert(_paletteTexture);
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
        if (!commandBuffer || !_gizmoShader || !_paletteTexture)
            return;

        // Start opaque pass for gizmos
        commandBuffer->beginOpaquePass(false); // false = don't clear depth buffer since we want gizmos on top

        // Set camera for gizmos
        commandBuffer->setViewProjection(camera.viewMatrix(), camera.projectionMatrix());

        // Bind shared shader and texture once for all gizmos
        commandBuffer->bind(_paletteTexture);
        commandBuffer->bind(_gizmoShader);

        // Render all gizmos that updated this frame
        for (const auto& weakGizmo : _frameGizmos)
        {
            auto gizmo = weakGizmo.lock();
            if (gizmo)
            {
                gizmo->renderGizmo(commandBuffer);
            }
        }

        // End opaque pass
        commandBuffer->endOpaquePass();

        // Clear frame gizmos after rendering
        _frameGizmos.clear();
    }


} // namespace noz::debug