/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/ISingleton.h>
#include <vector>
#include <memory>

namespace noz::renderer
{
	class CommandBuffer;
	class Shader;
	class Texture;
}

namespace noz::node
{
	class Camera;
}

namespace noz::debug
{
	class IGizmo;

	/**
	 * @brief System for managing and rendering debug gizmos
	 * Provides centralized rendering of all gizmo types in the scene
	 */
	class Gizmos : public noz::ISingleton<Gizmos>
	{
	public:
		Gizmos();
		~Gizmos();

		// Update all registered gizmos
		void update();

		// Render all enabled gizmos
		void render(noz::renderer::CommandBuffer* commandBuffer, const noz::node::Camera& camera);


		// Static load/unload methods for singleton pattern
		static void load();
		static void unload();

		// Shared resources for gizmos
		std::shared_ptr<noz::renderer::Shader> gizmoShader() const { return _gizmoShader; }
		std::shared_ptr<noz::renderer::Texture> paletteTexture() const { return _paletteTexture; }

		// Static UV coordinates for standard colors in the palette texture
		static glm::vec2 s_redColorUV;
		static glm::vec2 s_greenColorUV;
		static glm::vec2 s_blueColorUV;

	private:
		friend class noz::ISingleton<Gizmos>;
		friend class IGizmo;

		// Private registration methods - only IGizmo can call these
		void registerGizmo(std::shared_ptr<IGizmo> gizmo);

		void loadInternal();
		void unloadInternal();

		// Frame-based gizmo list - populated during update, cleared after render
		std::vector<std::weak_ptr<IGizmo>> _frameGizmos;
		std::shared_ptr<noz::renderer::Shader> _gizmoShader;
		std::shared_ptr<noz::renderer::Texture> _paletteTexture;
	};

} // namespace noz::debug