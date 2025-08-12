#pragma once

namespace noz::renderer
{
	class Texture;
	class Shader;
	class CommandBuffer;
	class Mesh;

	class PixelationEffect
	{
	public:
		PixelationEffect();
		~PixelationEffect();

		// Initialize the effect with reference size
		bool initialize(int referenceWidth, int referenceHeight);
		
		// Update for screen size changes
		void updateScreenSize(int screenWidth, int screenHeight);
		void setReferenceSize(int width, int height);
		
		// Get the render target texture to render the scene into
		std::shared_ptr<Texture> renderTarget() const { return _renderTarget; }
		
		// Begin rendering to the pixelation render target
		void beginRender(CommandBuffer* commandBuffer);
		
		// End rendering and apply pixelation effect to screen
		void endRender(CommandBuffer* commandBuffer);
		
		// Get parameters
		int referenceWidth() const { return _referenceWidth; }
		int referenceHeight() const { return _referenceHeight; }
		int renderTargetWidth() const { return _renderTargetWidth; }
		int renderTargetHeight() const { return _renderTargetHeight; }
		int screenWidth() const { return _screenWidth; }
		int screenHeight() const { return _screenHeight; }
		float scale() const { return _scale; }
		
		// Check if effect is enabled
		bool isEnabled() const { return _enabled; }
		void setEnabled(bool enabled) { _enabled = enabled; }

	private:
		void calculateRenderTargetSize();
		void createRenderTarget();
		void createFullscreenQuad();
		
		std::shared_ptr<Texture> _renderTarget;
		std::shared_ptr<Material> _material;
		std::shared_ptr<Mesh> _fullscreenQuad;
		
		// Reference resolution (the "pixel perfect" resolution we want)
		int _referenceWidth;
		int _referenceHeight;
		
		// Current screen size
		int _screenWidth;
		int _screenHeight;
		
		// Calculated render target size (maintains aspect ratio, closest to integer scale)
		int _renderTargetWidth;
		int _renderTargetHeight;
		float _scale;  // The scale factor used
		
		bool _enabled;
		bool _initialized;
	};
}