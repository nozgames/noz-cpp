/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "PipelineFactory.h"
#include "noz/Renderer/SamplerFactory.h"

namespace noz::renderer
{
    Renderer::Renderer()
        : _window(nullptr)
        , _gpu(nullptr)
        , _defaultSampler(nullptr)
        , _pointSampler(nullptr)
        , _currentCommandBuffer(nullptr)
        , _currentRenderPass(nullptr)
        , _initialized(false)
        , _frameInProgress(false)
        , _depthTexture(nullptr)
        , _depthWidth(0)
        , _depthHeight(0)
        , _msaaColorTexture(nullptr)
        , _msaaDepthTexture(nullptr)
        , _defaultTexture(nullptr)
        , _shadowMap(nullptr)
        , _shadowSampler(nullptr)
        , _shadowPassActive(false)
        , _msaaActive(false)
        , _commandBuffer(std::make_unique<CommandBuffer>())
        , _samplerFactory(nullptr)
        , _currentPipeline(nullptr)
        , _currentTexture(nullptr)
        , _currentTransform(1.0f)
        , _lightViewProjectionMatrix(1.0f)
    {
        // Light view projection will be set externally via setLightViewProjection
    }

    Renderer::~Renderer()
    {
        // OnUnload will be called by ISingleton::Unload()
    }

    bool Renderer::load(SDL_Window* window)
    {
		noz::ISingleton<Renderer>::load();
    
        // Set the window after the instance is created
		if (!instance()->SetWindow(window))
			return false;

		instance()->_shadowShader = Asset::load<Shader>("shaders/shadow");

		if (!instance()->_shadowShader)
		{
			std::cerr << "Failed to load shadow shader!" << std::endl;
			return false;
		}

		return true;
    }

    bool Renderer::SetWindow(SDL_Window* window)
    {
        _window = window;
    
        if (_initialized)
            return true;

        // Create GPU device
        _gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
        if (!_gpu)
        {
            std::cerr << "SDL_CreateGPUDevice Error: " << SDL_GetError() << std::endl;
            return false;
        }

        // Claim window for GPU device
        if (!SDL_ClaimWindowForGPUDevice(_gpu, _window))
        {
            std::cerr << "SDL_ClaimWindowForGPUDevice Error: " << SDL_GetError() << std::endl;
            SDL_DestroyGPUDevice(_gpu);
            _gpu = nullptr;
            return false;
        }

        // Create sampler factory
        _samplerFactory = std::make_unique<SamplerFactory>(_gpu);

        // Create default sampler
        SDL_GPUSamplerCreateInfo samplerInfo = {};
        samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
		samplerInfo.enable_compare = true;
		samplerInfo.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

        _defaultSampler = SDL_CreateGPUSampler(_gpu, &samplerInfo);
        if (!_defaultSampler)
        {
            std::cerr << "Failed to create default sampler: " << SDL_GetError() << std::endl;
            SDL_DestroyGPUDevice(_gpu);
            _gpu = nullptr;
            return false;
        }

        // Create point sampler for pixelation effect
        SDL_GPUSamplerCreateInfo pointSamplerInfo = {};
        pointSamplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
        pointSamplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
        pointSamplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        pointSamplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        pointSamplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        pointSamplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        _pointSampler = SDL_CreateGPUSampler(_gpu, &pointSamplerInfo);
        if (!_pointSampler)
        {
            std::cerr << "Failed to create point sampler: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUSampler(_gpu, _defaultSampler);
            SDL_DestroyGPUDevice(_gpu);
            _gpu = nullptr;
            return false;
        }

        // Create default texture
        _defaultTexture = Asset::load<noz::renderer::Texture>("white");
        if (!_defaultTexture)
        {
            std::cerr << "Failed to create default texture: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUSampler(_gpu, _defaultSampler);
            SDL_DestroyGPUDevice(_gpu);
            _gpu = nullptr;
            return false;
        }

        // Create shadow map using D32_FLOAT format for depth writing and sampling
        SDL_GPUTextureCreateInfo shadowInfo = {};
        shadowInfo.type = SDL_GPU_TEXTURETYPE_2D;
        shadowInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT; // Depth format for depth-stencil target
        shadowInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
        shadowInfo.width = SHADOW_MAP_SIZE;
        shadowInfo.height = SHADOW_MAP_SIZE;
        shadowInfo.layer_count_or_depth = 1;
        shadowInfo.num_levels = 1;
        shadowInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
        shadowInfo.props = SDL_CreateProperties();
		SDL_SetStringProperty(shadowInfo.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, "ShadowMap");

        _shadowMap = SDL_CreateGPUTexture(_gpu, &shadowInfo);
        if (!_shadowMap)
        {
            std::cerr << "Failed to create shadow map: " << SDL_GetError() << std::endl;
            _defaultTexture = nullptr;
            SDL_ReleaseGPUSampler(_gpu, _pointSampler);
            SDL_ReleaseGPUSampler(_gpu, _defaultSampler);
            SDL_DestroyGPUDevice(_gpu);
            _gpu = nullptr;
            return false;
        }

        // Create shadow sampler with depth comparison
		SDL_GPUSamplerCreateInfo shadowSamplerInfo = {};
		shadowSamplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
		shadowSamplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
		shadowSamplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
		shadowSamplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
		shadowSamplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
		shadowSamplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
		shadowSamplerInfo.enable_compare = true;
		shadowSamplerInfo.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

        _shadowSampler = SDL_CreateGPUSampler(_gpu, &shadowSamplerInfo);
        if (!_shadowSampler)
        {
            std::cerr << "Failed to create shadow sampler: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUTexture(_gpu, _shadowMap);
            _shadowMap = nullptr;
            _defaultTexture = nullptr;
            SDL_ReleaseGPUSampler(_gpu, _pointSampler);
            SDL_ReleaseGPUSampler(_gpu, _defaultSampler);
            SDL_DestroyGPUDevice(_gpu);
            _gpu = nullptr;
            return false;
        }

		SDL_DestroyProperties(shadowInfo.props);

        _initialized = true;
        return true;
    }

    void Renderer::unload()
    {
		instance()->unloadInternal();
		ISingleton<Renderer>::unload();
    }

	void Renderer::unloadInternal()
	{
		if (!_initialized)
			return;

		// Release depth textures first
		if (_depthTexture)
		{
			SDL_ReleaseGPUTexture(_gpu, _depthTexture);
			_depthTexture = nullptr;
		}
		
		// Release MSAA textures
		if (_msaaColorTexture)
		{
			SDL_ReleaseGPUTexture(_gpu, _msaaColorTexture);
			_msaaColorTexture = nullptr;
		}
		
		if (_msaaDepthTexture)
		{
			SDL_ReleaseGPUTexture(_gpu, _msaaDepthTexture);
			_msaaDepthTexture = nullptr;
		}

		// Release shadow resources
		if (_shadowMap)
		{
			SDL_ReleaseGPUTexture(_gpu, _shadowMap);
			_shadowMap = nullptr;
		}

		if (_shadowSampler)
		{
			SDL_ReleaseGPUSampler(_gpu, _shadowSampler);
			_shadowSampler = nullptr;
		}

		// Release default texture
		_defaultTexture.reset();

		// Cleanup sampler factory
		if (_samplerFactory)
		{
			_samplerFactory->cleanup();
			_samplerFactory.reset();
		}

		// Release samplers before destroying GPU device
		if (_pointSampler)
		{
			SDL_ReleaseGPUSampler(_gpu, _pointSampler);
			_pointSampler = nullptr;
		}
		
		if (_defaultSampler)
		{
			SDL_ReleaseGPUSampler(_gpu, _defaultSampler);
			_defaultSampler = nullptr;
		}

		// Destroy GPU device last
		if (_gpu)
		{
			SDL_DestroyGPUDevice(_gpu);
			_gpu = nullptr;
		}

		_window = nullptr;
		_currentCommandBuffer = nullptr;
		_currentRenderPass = nullptr;
		_frameInProgress = false;
		_shadowPassActive = false;
		_initialized = false;


	}

    // New CommandBuffer-based rendering methods
    CommandBuffer* Renderer::beginFrame()
    {
        if (!beginFrameImmediate())
        {
            std::cout << "beginFrameImmediate failed" << std::endl;
            return nullptr;
        }
        
        // Reset command buffer for new frame
        _commandBuffer->reset();
        return _commandBuffer.get();
    }

    void Renderer::endFrame()
    {
        // Execute the command buffer and then end the frame
        executeCommandBuffer(*_commandBuffer);
        endFrameImmediate();
    }

    void Renderer::executeCommandBuffer(const CommandBuffer& commandBuffer)
    {
        const auto& commands = commandBuffer.getCommands();
        
        for (const auto& command : commands)
        {
            switch (command.type)
            {
                case CommandType::BindTexture:
                {
                    const auto& data = std::get<BindTextureData>(command.data);
                    auto texture = commandBuffer.getTexture(data.textureHandle);
                    if (texture) bindTexture(texture);
                    break;
                }

                case CommandType::BindTextureWithSampler:
                {
                    const auto& data = std::get<BindTextureWithSamplerData>(command.data);
                    auto texture = commandBuffer.getTexture(data.textureHandle);
                    if (texture && data.sampler) bindTextureWithSampler(texture, data.sampler);
                    break;
                }
                
                case CommandType::BindShader:
                {
                    const auto& data = std::get<BindShaderData>(command.data);
                    auto shader = commandBuffer.getShader(data.shaderHandle);
                    if (shader) bindPipeline(shader);
                    break;
                }
                
                case CommandType::BindMesh:
                {
                    const auto& data = std::get<BindMeshData>(command.data);
                    auto mesh = commandBuffer.getMesh(data.meshHandle);
                    if (mesh && _currentRenderPass) mesh->bind(_currentRenderPass);
                    break;
                }
                
                case CommandType::SetTransform:
                {
                    const auto& data = std::get<SetTransformData>(command.data);
                    bindTransform(data.transform);
                    break;
                }
                
                case CommandType::SetCamera:
                {
                    const auto& data = std::get<SetCameraData>(command.data);
                    // Directly bind camera matrices to CameraBuffer (vs_b0, space1)
                    struct CameraBufferData
                    {
                        glm::mat4 vp;
                        glm::mat4 v;
                        glm::mat4 lightViewProjection;
                    };
                    
                    CameraBufferData cameraData = {
                        data.viewProjectionMatrix,
                        data.viewMatrix,
                        _lightViewProjectionMatrix
                    };
                    
                    SDL_PushGPUVertexUniformData(_currentCommandBuffer, 0, &cameraData, sizeof(CameraBufferData));
                    
                    // Store for legacy compatibility (still needed by bindTransform for ObjectBuffer)
                    _viewMatrix = data.viewMatrix;
                    _viewProjectionMatrix = data.viewProjectionMatrix;

					if (_shadowPassActive)
						_lightViewProjectionMatrix = _viewProjectionMatrix;

                    break;
                }
                
                case CommandType::SetBones:
                {
                    const auto& data = std::get<SetBonesData>(command.data);
					bindBones(commandBuffer.getBones(data.boneDataOffset, data.boneCount), data.boneCount);
                    break;
                }
                
                case CommandType::BindLight:
                {
                    const auto& data = std::get<BindLightData>(command.data);
                    // Push light data to fragment shader uniform buffer
                    SDL_PushGPUFragmentUniformData(
                        _currentCommandBuffer,
                        0,
                        &data,
                        sizeof(BindLightData));
                    break;
                }
                
                case CommandType::SetColor:
                {
                    const auto& data = std::get<SetColorData>(command.data);
                    // Push color data to fragment shader uniform buffer (register ps_b0, space3)
                    SDL_PushGPUFragmentUniformData(
                        _currentCommandBuffer,
                        0, // space3
                        &data,
                        sizeof(SetColorData));
                    break;
                }
                
                case CommandType::SetTextOptions:
                {
                    const auto& data = std::get<SetTextOptionsData>(command.data);
                    // Push text options data to fragment shader uniform buffer (register ps_b0, space3)
                    SDL_PushGPUFragmentUniformData(
                        _currentCommandBuffer,
                        0, // space3
                        &data,
                        sizeof(SetTextOptionsData));
                    break;
                }
                
                case CommandType::SetGridData:
                {
                    const auto& data = std::get<SetGridDataData>(command.data);
                    // Push grid data to vertex shader uniform buffer (register vs_b2, space1)
                    SDL_PushGPUVertexUniformData(
                        _currentCommandBuffer,
                        2, // vs_b2, space1
                        &data,
                        sizeof(SetGridDataData));
                    break;
                }
                                
                case CommandType::DrawMesh:
                {
                    const auto& data = std::get<DrawMeshData>(command.data);
                    auto mesh = commandBuffer.getMesh(data.meshHandle);
                    if (mesh && _currentRenderPass)
                    {
                        mesh->bind(_currentRenderPass);
                        mesh->draw(_currentRenderPass);
                    }
                    break;
                }
                
                case CommandType::BeginOpaquePass:
                {
                    const auto& data = std::get<BeginOpaquePassData>(command.data);
                    beginOpaquePass(data.clear, data.color, data.useMSAA);
                    break;
                }

                case CommandType::BeginOpaquePassWithTarget:
                {
                    const auto& data = std::get<BeginOpaquePassWithTargetData>(command.data);
                    auto renderTarget = commandBuffer.getTexture(data.renderTargetHandle);
					assert(renderTarget);
                    beginOpaquePass(renderTarget, data.clear, data.color);
                    break;
                }
                
                case CommandType::EndOpaquePass:
                {
                    endOpaquePass();
                    break;
                }
                
                case CommandType::BeginShadowPass:
                    beginShadowPass();
                    break;
                
                case CommandType::EndShadowPass:
                {
                    endShadowPass();
                    break;
                }
                
                case CommandType::SetViewport:
                {
                    const auto& data = std::get<SetViewportData>(command.data);
                    SDL_GPUViewport viewport = {};
                    viewport.x = static_cast<float>(data.x);
                    viewport.y = static_cast<float>(data.y);
                    viewport.w = static_cast<float>(data.width);
                    viewport.h = static_cast<float>(data.height);
                    viewport.min_depth = 0.0f;
                    viewport.max_depth = 1.0f;
                    if (_currentRenderPass)
                    {
                        SDL_SetGPUViewport(_currentRenderPass, &viewport);
                    }
                    break;
                }
                
                case CommandType::SetScissor:
                {
                    const auto& data = std::get<SetScissorData>(command.data);
                    SDL_Rect scissor = {data.x, data.y, data.width, data.height};
                    if (_currentRenderPass)
                    {
                        SDL_SetGPUScissor(_currentRenderPass, &scissor);
                    }
                    break;
                }
            }
        }
    }

    // Legacy immediate mode methods (renamed for clarity)
    bool Renderer::beginFrameImmediate()
    {
        if (!_initialized || _frameInProgress)
        {
            return false;
        }

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(_gpu);
        SDL_GPUTexture* backbuffer = nullptr;
        Uint32 width, height;
        SDL_WaitAndAcquireGPUSwapchainTexture(cmd, _window, &_backBuffer, &width, &height);
        if (!_backBuffer)
        {
            SDL_CancelGPUCommandBuffer(cmd);
            return false;
        }
        
        // Ensure we have valid dimensions
        if (width == 0 || height == 0)
        {
            std::cerr << "Invalid backbuffer dimensions: " << width << "x" << height << std::endl;
            
            // Try to get window dimensions as fallback
            int windowWidth, windowHeight;
            SDL_GetWindowSize(_window, &windowWidth, &windowHeight);
            std::cout << "Window dimensions: " << windowWidth << "x" << windowHeight << std::endl;
            
            if (windowWidth > 0 && windowHeight > 0)
            {
                width = (Uint32)windowWidth;
                height = (Uint32)windowHeight;
                std::cout << "Using window dimensions as fallback: " << width << "x" << height << std::endl;
            }
            else
            {
                std::cerr << "Window dimensions also invalid, using default size" << std::endl;
                width = 800;
                height = 600;
            }
        }

        // Create depth texture if it doesn't exist or if size changed
        if (!_depthTexture || _depthWidth != (int)width || _depthHeight != (int)height)
        {
            if (_depthTexture)
            {
                SDL_ReleaseGPUTexture(_gpu, _depthTexture);
                _depthTexture = nullptr;
            }

            // Create property group for D3D12 clear depth value to match our clear value
            SDL_PropertiesID depthProps = SDL_CreateProperties();
            SDL_SetFloatProperty(depthProps, SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT, 1.0f);

            SDL_GPUTextureCreateInfo depthInfo = {};
            depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
            depthInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
            depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
            depthInfo.width = width;
            depthInfo.height = height;
            depthInfo.layer_count_or_depth = 1;
            depthInfo.num_levels = 1;
            depthInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
            depthInfo.props = depthProps;  // Use the D3D12 properties

            _depthTexture = SDL_CreateGPUTexture(_gpu, &depthInfo);
            if (!_depthTexture)
            {
                std::cerr << "Failed to create depth texture: " << SDL_GetError() << std::endl;
                SDL_DestroyProperties(depthProps);
                SDL_CancelGPUCommandBuffer(cmd);
                return false;
            }

            SDL_DestroyProperties(depthProps);

            // Create MSAA textures
            if (_msaaColorTexture)
            {
                SDL_ReleaseGPUTexture(_gpu, _msaaColorTexture);
                _msaaColorTexture = nullptr;
            }
            if (_msaaDepthTexture)
            {
                SDL_ReleaseGPUTexture(_gpu, _msaaDepthTexture);
                _msaaDepthTexture = nullptr;
            }

            // Create MSAA color texture
            SDL_GPUTextureCreateInfo msaaColorInfo = {};
            msaaColorInfo.type = SDL_GPU_TEXTURETYPE_2D;
            msaaColorInfo.format = SDL_GetGPUSwapchainTextureFormat(_gpu, _window);
            msaaColorInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
            msaaColorInfo.width = width;
            msaaColorInfo.height = height;
            msaaColorInfo.layer_count_or_depth = 1;
            msaaColorInfo.num_levels = 1;
            msaaColorInfo.sample_count = SDL_GPU_SAMPLECOUNT_4; // 4x MSAA
            msaaColorInfo.props = SDL_CreateProperties();
            SDL_SetStringProperty(msaaColorInfo.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, "MSAAColor");

            _msaaColorTexture = SDL_CreateGPUTexture(_gpu, &msaaColorInfo);
            if (!_msaaColorTexture)
            {
                std::cerr << "Failed to create MSAA color texture: " << SDL_GetError() << std::endl;
                SDL_DestroyProperties(msaaColorInfo.props);
                SDL_CancelGPUCommandBuffer(cmd);
                return false;
            }
            SDL_DestroyProperties(msaaColorInfo.props);

            // Create MSAA depth texture  
            SDL_PropertiesID msaaDepthProps = SDL_CreateProperties();
            SDL_SetFloatProperty(msaaDepthProps, SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT, 1.0f);

            SDL_GPUTextureCreateInfo msaaDepthInfo = {};
            msaaDepthInfo.type = SDL_GPU_TEXTURETYPE_2D;
            msaaDepthInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
            msaaDepthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
            msaaDepthInfo.width = width;
            msaaDepthInfo.height = height;
            msaaDepthInfo.layer_count_or_depth = 1;
            msaaDepthInfo.num_levels = 1;
            msaaDepthInfo.sample_count = SDL_GPU_SAMPLECOUNT_4; // 4x MSAA
            msaaDepthInfo.props = msaaDepthProps;

            _msaaDepthTexture = SDL_CreateGPUTexture(_gpu, &msaaDepthInfo);
            if (!_msaaDepthTexture)
            {
                std::cerr << "Failed to create MSAA depth texture: " << SDL_GetError() << std::endl;
                SDL_DestroyProperties(msaaDepthProps);
                SDL_CancelGPUCommandBuffer(cmd);
                return false;
            }
            SDL_DestroyProperties(msaaDepthProps);

            _depthWidth = (int)width;
            _depthHeight = (int)height;
        }

        _currentCommandBuffer = cmd;
        _frameInProgress = true;
        return true;
    }

    void Renderer::endFrameImmediate()
    {
		assert(_currentCommandBuffer);

        SDL_SubmitGPUCommandBuffer(_currentCommandBuffer);
    
        _currentCommandBuffer = nullptr;
        _currentRenderPass = nullptr;
        _frameInProgress = false;
    }

    bool Renderer::beginOpaquePass(bool clear, Color clearColor)
    {
        // Default to no MSAA for backward compatibility
        return beginOpaquePass(clear, clearColor, false);
    }
    
    bool Renderer::beginOpaquePass(bool clear, Color clearColor, bool useMSAA)
    {
		assert(!_currentRenderPass);

        SDL_GPUColorTargetInfo colorTargetInfo = {};
        SDL_GPUDepthStencilTargetInfo depthTargetInfo = {};
        
        if (useMSAA && _msaaColorTexture && _msaaDepthTexture)
        {
            // Use MSAA textures for scene rendering with resolve to backbuffer
            _msaaActive = true;
            colorTargetInfo.texture = _msaaColorTexture;
            colorTargetInfo.clear_color = clearColor;
            colorTargetInfo.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
            colorTargetInfo.store_op = SDL_GPU_STOREOP_RESOLVE; // Resolve MSAA to backbuffer
            colorTargetInfo.resolve_texture = _backBuffer; // Resolve target
            colorTargetInfo.resolve_mip_level = 0;
            colorTargetInfo.resolve_layer = 0;

            depthTargetInfo.texture = _msaaDepthTexture;
            depthTargetInfo.clear_depth = 1.0f;
            depthTargetInfo.clear_stencil = 0;
            depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE; // Don't need to store MSAA depth
        }
        else
        {
            // Standard rendering without MSAA
            _msaaActive = false;
            colorTargetInfo.texture = _backBuffer;
            colorTargetInfo.clear_color = clearColor;
            colorTargetInfo.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
            colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

            depthTargetInfo.texture = _depthTexture;
            depthTargetInfo.clear_depth = 1.0f;
            depthTargetInfo.clear_stencil = 0;
            depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            depthTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
        }

        _currentRenderPass = SDL_BeginGPURenderPass(_currentCommandBuffer, &colorTargetInfo, 1, &depthTargetInfo);
        if (!_currentRenderPass)
            return false;

		resetState();

        return true;
    }

    bool Renderer::beginOpaquePass(const std::shared_ptr<Texture>& renderTarget, bool clear, Color clearColor)
    {
        assert(!_currentRenderPass);
		assert(renderTarget);
        
        // Set up render pass to render to the provided texture
        SDL_GPUColorTargetInfo colorTargetInfo = {};
        colorTargetInfo.texture = renderTarget->handle();
        colorTargetInfo.clear_color = clearColor; // Transparent background for icon
        colorTargetInfo.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
        
        SDL_GPUDepthStencilTargetInfo depthTargetInfo = {};
        depthTargetInfo.texture = _depthTexture;
        depthTargetInfo.clear_depth = 1.0f;
        depthTargetInfo.clear_stencil = 0;
        depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE; // Don't need to store depth for render target
        
        _currentRenderPass = SDL_BeginGPURenderPass(_currentCommandBuffer, &colorTargetInfo, 1, &depthTargetInfo);
        if (!_currentRenderPass)
            return false;
            
        resetState();
        return true;
    }

    void Renderer::endOpaquePass()
    {
        if (!_currentRenderPass)
            return;

        SDL_EndGPURenderPass(_currentRenderPass);
        _currentRenderPass = nullptr;
        _msaaActive = false;
    }

    void Renderer::bindTexture(const std::shared_ptr<Texture>& texture)
    {
		// Shadow pass: no texture samplers needed (depth-only rendering)
		if (_shadowPassActive)
			return;

		// Get the actual texture to bind (use default if none provided)
		auto actualTexture = texture ? texture : _defaultTexture;
		
		// Only bind if texture changed
		if (_currentTexture == actualTexture)
			return;

		// Get the appropriate sampler for this texture from the factory
		SDL_GPUSampler* sampler = _defaultSampler;
		if (_samplerFactory && actualTexture)
		{
			SDL_GPUSampler* textureSampler = _samplerFactory->getSampler(actualTexture->samplerOptions());
			if (textureSampler)
				sampler = textureSampler;
		}

		// Main pass: bind diffuse texture and shadow map
		SDL_GPUTextureSamplerBinding bindings[2] { 0 };
		int bindingCount = 0;
		
		// Bind diffuse texture to slot 0
		bindings[bindingCount].sampler = sampler;
		bindings[bindingCount].texture = actualTexture->handle();
		bindingCount++;
			
		// Bind shadow map to slot 1 (comparison sampler)
		if (_shadowMap && _shadowSampler)
		{
			bindings[bindingCount].sampler = _shadowSampler;
			bindings[bindingCount].texture = _shadowMap;
			bindingCount++;
		}
				
		SDL_BindGPUFragmentSamplers(_currentRenderPass, 0, bindings, bindingCount);
		_currentTexture = actualTexture;
    }

    void Renderer::bindTextureWithSampler(const std::shared_ptr<Texture>& texture, SDL_GPUSampler* sampler)
    {
		// Shadow pass: no texture samplers needed (depth-only rendering)
		if (_shadowPassActive)
			return;

		// Get the actual texture to bind (use default if none provided)
		auto actualTexture = texture ? texture : _defaultTexture;
		auto actualSampler = sampler ? sampler : _defaultSampler;
		
		// Main pass: bind diffuse texture with specified sampler
		SDL_GPUTextureSamplerBinding bindings[2] { 0 };
		int bindingCount = 0;
		
		// Bind diffuse texture to slot 0 with specified sampler
		bindings[bindingCount].sampler = actualSampler;
		bindings[bindingCount].texture = actualTexture->handle();
		bindingCount++;
			
		// Bind shadow map to slot 1 (comparison sampler)
		if (_shadowMap && _shadowSampler)
		{
			bindings[bindingCount].sampler = _shadowSampler;
			bindings[bindingCount].texture = _shadowMap;
			bindingCount++;
		}
				
		SDL_BindGPUFragmentSamplers(_currentRenderPass, 0, bindings, bindingCount);
		_currentTexture = actualTexture;
    }
    
    void Renderer::bindPipeline(const std::shared_ptr<Shader>& shader)
    {
        assert(shader);		

        auto* pipeline = PipelineFactory::getOrCreatePipeline(_gpu, _shadowPassActive ? _shadowShader : shader);
        if (!pipeline)
            return;

        // Only bind if pipeline changed
		if (_currentPipeline == pipeline)
			return;

		SDL_BindGPUGraphicsPipeline(_currentRenderPass, pipeline);
        _currentPipeline = pipeline;
    }

    void Renderer::bindTransform(const glm::float4x4& transform)
    {
        struct ObjectBufferData
        {
            glm::mat4 m;
        };

        ObjectBufferData objectData = 
		{
            transform
        };

        SDL_PushGPUVertexUniformData(_currentCommandBuffer, 1, &objectData, sizeof(ObjectBufferData));
        _currentTransform = transform;
    }

	void Renderer::bindBones(const glm::mat4* bones, int boneCount)
	{
		assert(bones);
		assert(boneCount > 0);

		SDL_PushGPUVertexUniformData(
			_currentCommandBuffer,
			2,
			bones,
			static_cast<Uint32>(boneCount * sizeof(glm::mat4)));
	}

	void Renderer::setLightDirection(const glm::vec3& direction)
	{
		// Store the light direction - used by external code to calculate light view projection
		// This method is kept for compatibility but the actual matrix calculation is external
	}

	bool Renderer::beginShadowPass()
	{
		assert(!_currentRenderPass);
		assert(_shadowMap);

		// Start shadow pass using depth-only rendering
		SDL_GPUDepthStencilTargetInfo shadowDepthTargetInfo = {};
		shadowDepthTargetInfo.texture = _shadowMap;
		shadowDepthTargetInfo.clear_depth = 1.0f;
		shadowDepthTargetInfo.clear_stencil = 0;
		shadowDepthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		shadowDepthTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

		_currentRenderPass = SDL_BeginGPURenderPass(_currentCommandBuffer, nullptr, 0, &shadowDepthTargetInfo);
		if (!_currentRenderPass)
			return false;		

		_shadowPassActive = true;
		resetState();
		return true;
	}

	void Renderer::endShadowPass()
	{
		assert(_shadowPassActive);
		assert(_currentRenderPass);

		SDL_EndGPURenderPass(_currentRenderPass);
		_currentRenderPass = nullptr;
		_shadowPassActive = false;
	}

	noz::Image Renderer::readTexturePixels(const std::shared_ptr<Texture>& texture)
	{
		if (!texture || !_gpu)
		{
			std::cerr << "Invalid texture or GPU device for pixel readback" << std::endl;
			return noz::Image();
		}

		int width = texture->width();
		int height = texture->height();
		
		// Create transfer buffer for reading back pixels
		SDL_GPUTransferBufferCreateInfo transferInfo = {};
		transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
		transferInfo.size = width * height * 4; // RGBA format
		transferInfo.props = 0;

		SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(_gpu, &transferInfo);
		if (!transferBuffer)
		{
			std::cerr << "Failed to create transfer buffer for texture readback: " << SDL_GetError() << std::endl;
			return noz::Image();
		}

		// Create command buffer for the copy operation
		SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(_gpu);
		if (!cmd)
		{
			std::cerr << "Failed to acquire command buffer for texture readback" << std::endl;
			SDL_ReleaseGPUTransferBuffer(_gpu, transferBuffer);
			return noz::Image();
		}

		// Copy texture to transfer buffer
		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
		
		SDL_GPUTextureRegion textureRegion = {};
		textureRegion.texture = texture->handle();
		textureRegion.w = width;
		textureRegion.h = height;
		textureRegion.d = 1;

		SDL_GPUTextureTransferInfo transferDst = {};
		transferDst.transfer_buffer = transferBuffer;
		transferDst.offset = 0;
		transferDst.pixels_per_row = width;
		transferDst.rows_per_layer = height;

		SDL_DownloadFromGPUTexture(copyPass, &textureRegion, &transferDst);
		SDL_EndGPUCopyPass(copyPass);

		// Submit command buffer and wait for completion
		SDL_SubmitGPUCommandBuffer(cmd);
		SDL_WaitForGPUIdle(_gpu);

		// Map transfer buffer to read the data
		void* mappedData = SDL_MapGPUTransferBuffer(_gpu, transferBuffer, false);
		if (!mappedData)
		{
			std::cerr << "Failed to map transfer buffer for texture readback: " << SDL_GetError() << std::endl;
			SDL_ReleaseGPUTransferBuffer(_gpu, transferBuffer);
			return noz::Image();
		}

		// Create Image and copy the pixel data
		noz::Image result(width, height, noz::Image::Format::RGBA);
		std::memcpy(result.data(), mappedData, width * height * 4);

		// Clean up
		SDL_UnmapGPUTransferBuffer(_gpu, transferBuffer);
		SDL_ReleaseGPUTransferBuffer(_gpu, transferBuffer);

		return result;
	}

	void Renderer::resetState()
	{
		// Reset all state tracking variables to force rebinding
		_currentPipeline = nullptr;
		_currentTexture = nullptr;
		_currentTransform = glm::float4x4(0.0f); // Use identity matrix as default
	}


} // namespace noz::renderer
