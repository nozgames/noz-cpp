#include <noz/renderer/effects/PixelationEffect.h>
#include <noz/renderer/Texture.h>
#include <noz/renderer/Shader.h>
#include <noz/renderer/Mesh.h>
#include <noz/renderer/CommandBuffer.h>
#include <noz/renderer/Renderer.h>
#include <noz/renderer/MeshBuilder.h>
#include <noz/Resources.h>

namespace noz::renderer
{
	PixelationEffect::PixelationEffect()
		: _referenceWidth(0)
		, _referenceHeight(0)
		, _screenWidth(0)
		, _screenHeight(0)
		, _renderTargetWidth(0)
		, _renderTargetHeight(0)
		, _scale(1.0f)
		, _enabled(true)
		, _initialized(false)
	{
	}

	PixelationEffect::~PixelationEffect()
	{
		_renderTarget.reset();
		_pixelationShader.reset();
		_fullscreenQuad.reset();
	}

	bool PixelationEffect::initialize(int referenceWidth, int referenceHeight)
	{
		_referenceWidth = referenceWidth;
		_referenceHeight = referenceHeight;
		
		// Load the pixelation shader
		_pixelationShader = Resources::instance()->load<Shader>("shaders/pixelation");
		if (!_pixelationShader)
		{
			std::cerr << "Failed to load pixelation shader" << std::endl;
			return false;
		}
		
		// Create fullscreen quad for rendering the effect
		createFullscreenQuad();
		if (!_fullscreenQuad)
		{
			std::cerr << "Failed to create fullscreen quad for pixelation effect" << std::endl;
			return false;
		}
		
		_initialized = true;
		return true;
	}

	void PixelationEffect::setReferenceSize(int width, int height)
	{
		if (_referenceWidth != width || _referenceHeight != height)
		{
			_referenceWidth = width;
			_referenceHeight = height;
			if (_screenWidth > 0 && _screenHeight > 0)
			{
				calculateRenderTargetSize();
				createRenderTarget();
			}
		}
	}

	void PixelationEffect::updateScreenSize(int screenWidth, int screenHeight)
	{
		if (_screenWidth != screenWidth || _screenHeight != screenHeight)
		{
			_screenWidth = screenWidth;
			_screenHeight = screenHeight;
			if (_referenceWidth > 0 && _referenceHeight > 0)
			{
				calculateRenderTargetSize();
				createRenderTarget();
			}
		}
	}

	void PixelationEffect::calculateRenderTargetSize()
	{
		if (_referenceWidth <= 0 || _referenceHeight <= 0 || _screenWidth <= 0 || _screenHeight <= 0)
		{
			_renderTargetWidth = _referenceWidth;
			_renderTargetHeight = _referenceHeight;
			_scale = 1.0f;
			return;
		}

		// Calculate aspect ratios
		float referenceAspect = static_cast<float>(_referenceWidth) / static_cast<float>(_referenceHeight);
		float screenAspect = static_cast<float>(_screenWidth) / static_cast<float>(_screenHeight);

		// Calculate potential scales for each dimension
		float scaleX = static_cast<float>(_screenWidth) / static_cast<float>(_referenceWidth);
		float scaleY = static_cast<float>(_screenHeight) / static_cast<float>(_referenceHeight);

		// Find the scale that's closest to an integer (for pixel-perfect scaling)
		float integerScaleX = std::round(scaleX);
		float integerScaleY = std::round(scaleY);

		// Calculate how close each scale is to an integer
		float distanceX = std::abs(scaleX - integerScaleX);
		float distanceY = std::abs(scaleY - integerScaleY);

		// Use the dimension that's closest to integer scaling
		if (distanceX <= distanceY)
		{
			// Use X dimension to determine scale
			_scale = std::max(1.0f, integerScaleX);
			_renderTargetWidth = static_cast<int>(static_cast<float>(_screenWidth) / _scale);
			_renderTargetHeight = static_cast<int>(static_cast<float>(_renderTargetWidth) / screenAspect);
		}
		else
		{
			// Use Y dimension to determine scale
			_scale = std::max(1.0f, integerScaleY);
			_renderTargetHeight = static_cast<int>(static_cast<float>(_screenHeight) / _scale);
			_renderTargetWidth = static_cast<int>(static_cast<float>(_renderTargetHeight) * screenAspect);
		}

		// Ensure we don't go below 1x1
		_renderTargetWidth = std::max(1, _renderTargetWidth);
		_renderTargetHeight = std::max(1, _renderTargetHeight);
		
		std::cout << "Pixelation: Screen " << _screenWidth << "x" << _screenHeight 
				  << " -> RenderTarget " << _renderTargetWidth << "x" << _renderTargetHeight 
				  << " (scale: " << _scale << ")" << std::endl;
	}

	void PixelationEffect::createRenderTarget()
	{
		auto* renderer = Renderer::instance();
		if (!renderer || !renderer->GetGPUDevice())
		{
			std::cerr << "Renderer not initialized for pixelation render target" << std::endl;
			return;
		}
		
		// Only create if we have valid dimensions
		if (_renderTargetWidth <= 0 || _renderTargetHeight <= 0)
		{
			return;
		}
		
		// Create render target at calculated resolution
		_renderTarget = std::shared_ptr<Texture>(
			Texture::createRenderTarget(
				renderer->GetGPUDevice(),
				_renderTargetWidth,
				_renderTargetHeight,
				"PixelationRenderTarget"
			)
		);
	}

	void PixelationEffect::createFullscreenQuad()
	{
		MeshBuilder builder;
		
		// Create a fullscreen quad (-1 to 1 in NDC space)
		// Positions
		builder.addVertex(vec3(-1.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(0.0f, 1.0f));
		builder.addVertex(vec3(1.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(1.0f, 1.0f));
		builder.addVertex(vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(1.0f, 0.0f));
		builder.addVertex(vec3(-1.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(0.0f, 0.0f));
		
		// Indices for two triangles
		builder.addTriangle(0, 1, 2);
		builder.addTriangle(0, 2, 3);
		
		_fullscreenQuad = builder.build("PixelationQuad");
	}

	void PixelationEffect::beginRender(CommandBuffer* commandBuffer)
	{
		if (!_initialized || !_enabled || !_renderTarget)
			return;
		
		// Begin rendering to our render target with clear
		commandBuffer->beginOpaquePass(_renderTarget, true, Color(0.0f, 0.0f, 0.0f, 1.0f));
	}

	void PixelationEffect::endRender(CommandBuffer* commandBuffer)
	{
		if (!_initialized || !_enabled || !_renderTarget || !_pixelationShader || !_fullscreenQuad)
			return;
		
		// End the render target pass
		commandBuffer->endOpaquePass();
		
		// Now render the pixelated result to the screen
		// Begin a new pass for the main framebuffer
		commandBuffer->beginOpaquePass(true, Color(0.0f, 0.0f, 0.0f, 1.0f));
		
		// Set up for fullscreen rendering
		// Use identity matrices since we're rendering in NDC space
		commandBuffer->setCamera(mat4(1.0f), mat4(1.0f), mat4(1.0f));
		commandBuffer->setTransform(mat4(1.0f));
		
		// Bind the pixelation shader
		commandBuffer->bindShader(_pixelationShader);
		
		// Bind the render target as texture with point sampling
		commandBuffer->bindTextureWithSampler(_renderTarget, Renderer::instance()->pointSampler());
		
		// Set pixelation parameters using render target dimensions
		// The shader will use these for pixel snapping calculations
		vec2 renderTargetSize = vec2(static_cast<float>(_renderTargetWidth), static_cast<float>(_renderTargetHeight));
		
		// Pass the render target size to the shader via the color buffer
		// Since we removed pixelSize, we just pass the texture size and a scale factor
		commandBuffer->setColor(vec4(renderTargetSize.x, renderTargetSize.y, _scale, 0.0f));
		
		// Draw the fullscreen quad
		commandBuffer->drawMesh(_fullscreenQuad);
		
		// End the screen pass
		commandBuffer->endOpaquePass();
	}
}