/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::node
{
    DirectionalLight::DirectionalLight()
        : Node3d()
        , _ambientColor(0.3f, 0.3f, 0.3f)
        , _diffuseColor(1.0f, 1.0f, 1.0f)
        , _ambientIntensity(0.3f)
        , _diffuseIntensity(1.0f)
    {
    }

    vec3 DirectionalLight::direction() const
    {
        return forward();
    }

    void DirectionalLight::update()
    {
        Node3d::update();

        // Register this light as the active directional light in the scene
        if (auto scenePtr = scene())
        {
            scenePtr->setActiveDirectionalLight(std::static_pointer_cast<DirectionalLight>(shared_from_this()));
        }
    }

	void DirectionalLight::beginShadowPass(noz::renderer::CommandBuffer* commandBuffer)
	{
		const float shadowMapSize = 15.0f; // Size of the shadow frustum
		const float shadowNear = 0.1f;
		const float shadowFar = 40.0f;
		
		// Create orthographic projection for shadow mapping
		auto lightProjection =
			glm::ortho(
				-shadowMapSize, shadowMapSize,
				-shadowMapSize, shadowMapSize,
				shadowNear, shadowFar);

		// Calculate optimal shadow camera position based on main camera
		// Position shadow camera to cover area around the main camera/player
		glm::vec3 shadowCameraPosition = glm::vec3(0.0f); // Default fallback
		
		if (auto mainCamera = scene()->camera())
		{
			// Get main camera's world position and forward direction
			glm::vec3 cameraPos = mainCamera->position();
			glm::vec3 cameraForward = mainCamera->forward();
			
			// Project camera forward to find intersection with ground plane (Y = 0)
			glm::vec3 focusPoint;
			if (cameraForward.y != 0.0f) 
			{
				// Calculate t where cameraPos + t * cameraForward intersects Y = 0
				float t = -cameraPos.y / cameraForward.y;
				focusPoint = cameraPos + t * cameraForward;
			}
			else
			{
				// Camera is looking horizontally, just use camera's X/Z at ground level
				focusPoint = glm::vec3(cameraPos.x, 0.0f, cameraPos.z);
			}
			
			// Position shadow camera to cover both the focus point and origin
			// Move back by half of shadowFar to center the depth range
			shadowCameraPosition = focusPoint - direction() * (shadowFar * 0.5f);
		}
		else
		{
			// Fallback if no main camera found
			shadowCameraPosition = -direction() * (shadowFar * 0.5f);
		}
		
		// Create view matrix for shadow camera looking towards the focus area
		glm::vec3 focusTarget = shadowCameraPosition + direction();
		if (auto mainCamera = scene()->camera())
		{
			// Get main camera's world position and forward direction
			glm::vec3 cameraPos = mainCamera->position();
			glm::vec3 cameraForward = mainCamera->forward();
			
			// Project camera forward to find intersection with ground plane (Y = 0)
			if (cameraForward.y != 0.0f) 
			{
				// Calculate t where cameraPos + t * cameraForward intersects Y = 0
				float t = -cameraPos.y / cameraForward.y;
				focusTarget = cameraPos + t * cameraForward;
			}
			else
			{
				// Camera is looking horizontally, just use camera's X/Z at ground level
				focusTarget = glm::vec3(cameraPos.x, 0.0f, cameraPos.z);
			}
		}
		
		auto lightView = glm::lookAt(
			shadowCameraPosition,
			focusTarget,
			glm::vec3(0.0f, 1.0f, 0.0f)
		);

		commandBuffer->beginShadowPass(lightView, lightProjection);
	}

	void DirectionalLight::endShadowPass(noz::renderer::CommandBuffer* commandBuffer)
	{
		commandBuffer->endShadowPass();
	}
}