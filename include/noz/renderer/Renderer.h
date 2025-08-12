#pragma once

namespace noz::node
{
	class Camera;
}

namespace noz::renderer
{
	class Shader;
	class Material;
	class Mesh;
	class Texture;
	class CommandBuffer;
	class SamplerFactory;

	namespace registers
	{
		enum class Vertex : uint32_t
		{
			Camera = 0,
			Object = 1,
			Bone = 2,
			User0 = 3,
			User1 = 4,
			User2 = 5,

			Count
		};

		enum class Fragment : uint32_t
		{
			Color = 0,
			Light = 1,
			User0 = 2,
			User1 = 3,
			User2 = 4,
			Count
		};

		enum class Sampler : uint32_t
		{
			ShadowMap = 0,
			User0 = 1,
			User1 = 2,
			User2 = 3,
			Count
		};
	}

	class Renderer : public noz::ISingleton<Renderer>
	{
	public:
		Renderer();
		~Renderer();

		bool SetWindow(SDL_Window* window);
		
		std::shared_ptr<Texture> defaultTexture() const { return _defaultTexture; }
		SDL_GPUSampler* pointSampler() const { return _pointSampler; }
		SDL_GPUSampler* linearSampler() const { return _defaultSampler; }
		SamplerFactory* samplerFactory() const { return _samplerFactory.get(); }

		// New CommandBuffer-based rendering
		CommandBuffer* beginFrame();
		void endFrame();
		void executeCommandBuffer(const CommandBuffer& commandBuffer);
				
		
		// Shadow mapping configuration
		void setLightDirection(const glm::vec3& direction);
		
		// Light configuration setters
		void setLightAmbientColor(const glm::vec3& color) { _lightAmbientColor = color; }
		void setLightDiffuseColor(const glm::vec3& color) { _lightDiffuseColor = color; }
		void setLightAmbientIntensity(float intensity) { _lightAmbientIntensity = intensity; }
		void setLightDiffuseIntensity(float intensity) { _lightDiffuseIntensity = intensity; }
		
		// Shadow mapping methods
		bool beginShadowPass();
		void endShadowPass();

		// State management
		void resetState();

		bool isShadowPassActive() const { return _shadowPassActive; }
		bool isMSAAActive() const { return _msaaActive; }

		// Texture readback
		noz::Image readTexturePixels(const std::shared_ptr<Texture>& texture);
		
		SDL_GPUDevice* device() const;
		SDL_GPUCommandBuffer* GetCommandBuffer() const { return static_cast<SDL_GPUCommandBuffer*>(_currentCommandBuffer); }
		SDL_GPURenderPass* GetCurrentRenderPass() const { return _currentRenderPass; }
		bool IsInitialized() const { return _initialized; }
		SDL_Window* GetWindow() const { return _window; }

		const glm::float4x4& viewProjection() const;
		
		static bool load(SDL_Window* window);
		static void unload();

	private:

		friend class noz::ISingleton<Renderer>;

		void unloadInternal();

		// Legacy immediate mode methods (deprecated)
		bool beginFrameImmediate();
		void endFrameImmediate();
		bool beginOpaquePass(bool clear, Color clearColor);
		bool beginOpaquePass(bool clear, Color clearColor, bool useMSAA);
		bool beginOpaquePass(const std::shared_ptr<Texture>& renderTarget, bool clear, Color clearColor, bool useMSAA);
		void endOpaquePass();

		// Bind default texture and sampler for rendering
		void bindTexture(const std::shared_ptr<Texture>& texture, int index = 0);
		void bindTextureWithSampler(const std::shared_ptr<Texture>& texture, SDL_GPUSampler* sampler);
		void bindPipeline(const std::shared_ptr<Shader>& shader);
		void bindMaterial(const std::shared_ptr<Material>& material);
		void bindTransform(const glm::float4x4& transform);
		void bindLight(const BindLightData& data);
		void bindColor(const BindColorData& color);

		void bindBones(const glm::mat4* bones, int boneCount);


		SDL_Window* _window;
		SDL_GPUDevice* _gpu;
		SDL_GPUSampler* _defaultSampler;
		SDL_GPUSampler* _pointSampler;  // Point sampling for pixelation effect
        SDL_GPUCommandBuffer* _currentCommandBuffer;
        SDL_GPURenderPass* _currentRenderPass;
		bool _initialized;
		bool _frameInProgress;
        glm::float4x4 _viewProjectionMatrix;
		glm::mat4 _viewMatrix;
		
		// Depth buffer support
		SDL_GPUTexture* _depthTexture;
		int _depthWidth;
		int _depthHeight;
		
		// MSAA support
		SDL_GPUTexture* _msaaColorTexture;
		SDL_GPUTexture* _msaaDepthTexture;
		
		// Default texture for rendering
		std::shared_ptr<noz::renderer::Texture> _defaultTexture;
		
		// Light view projection matrix for shadow mapping
		glm::mat4 _lightViewProjectionMatrix;
		glm::vec3 _lightAmbientColor = glm::vec3(0.3f, 0.3f, 0.3f);
		glm::vec3 _lightDiffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
		float _lightAmbientIntensity = 0.3f;
		float _lightDiffuseIntensity = 1.0f;
		
		// Shadow mapping
		SDL_GPUTexture* _backBuffer;
		SDL_GPUTexture* _shadowMap;           // Shadow map texture (D32_FLOAT format for depth writing and sampling)
		SDL_GPUSampler* _shadowSampler;
		std::shared_ptr<Shader> _shadowShader;
		bool _shadowPassActive;
		bool _msaaActive;
		static const int SHADOW_MAP_SIZE = 2048;

		// CommandBuffer support
		std::unique_ptr<CommandBuffer> _commandBuffer;
		
		// Sampler factory for texture-specific samplers
		std::unique_ptr<SamplerFactory> _samplerFactory;
		
		// State tracking to avoid redundant bindings
		SDL_GPUGraphicsPipeline* _currentPipeline;
		glm::float4x4 _currentTransform;
	};

	inline const glm::float4x4& Renderer::viewProjection() const 
	{
		return _viewProjectionMatrix;
	}

	inline SDL_GPUDevice* Renderer::device() const
	{
		return _gpu;
	}
}

