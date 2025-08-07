/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "PipelineFactory.h"

namespace noz::renderer
{
    std::unordered_map<size_t, SDL_GPUGraphicsPipeline*> PipelineFactory::pipelineCache;

    SDL_GPUGraphicsPipeline* PipelineFactory::getOrCreatePipeline(
        SDL_GPUDevice* device,
        const std::shared_ptr<Shader>& shader)
    {
		assert(device);
		assert(shader);

        size_t key = generatePipelineKey(shader);
        
        // Check cache first
        auto it = pipelineCache.find(key);
        if (it != pipelineCache.end())
            return it->second;
        
        // Create new pipeline
        std::vector<SDL_GPUVertexAttribute> attributes = 
            {
                {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0},  // position : POSITION (semantic 0)
                {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(float) * 3}, // uv0 : TEXCOORD1 (semantic 2)
                {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, sizeof(float) * 5}, // normal : TEXCOORD2 (semantic 1)
                {3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(float) * 8}  // boneIndex : TEXCOORD3 (semantic 3)
            };
        
        // DEBUG: Clear cache to force recreation with fixed shaders
        pipelineCache.clear();
        
        SDL_GPUGraphicsPipeline* pipeline = createGraphicsPipeline(device, shader, attributes);        
        if (pipeline) 
            pipelineCache[key] = pipeline;
        
        return pipeline;
    }

    size_t PipelineFactory::generatePipelineKey(const std::shared_ptr<Shader>& shader)
    {
        // Combine shader pointer and MSAA state to create a unique key
        std::hash<const void*> hasher;
        std::hash<bool> boolHasher;
        
        auto* renderer = Renderer::instance();
        bool useMSAA = renderer ? renderer->isMSAAActive() : false;
        
        // Combine shader pointer and MSAA flag
        size_t h1 = hasher(shader.get());
        size_t h2 = boolHasher(useMSAA);
        return h1 ^ (h2 << 1);
    }

    SDL_GPUGraphicsPipeline* PipelineFactory::createGraphicsPipeline(
        SDL_GPUDevice* device,
        const std::shared_ptr<Shader>& shader,
        const std::vector<SDL_GPUVertexAttribute>& attributes)
    {
        if (!device || !shader)
            return nullptr;

        // Check if this is an animated shader by looking at the path
        std::string shaderPath = shader->name();
        
        // Create pipeline directly using the shader's compiled shaders
        auto vertexStride = getVertexStride(attributes);
        
        SDL_GPUVertexBufferDescription vertexBufferDesc = {};
        vertexBufferDesc.pitch = vertexStride;
        vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        SDL_GPUVertexInputState vertexInputState = {};
        vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
        vertexInputState.num_vertex_buffers = 1;
        vertexInputState.vertex_attributes = attributes.data();
        vertexInputState.num_vertex_attributes = static_cast<Uint32>(attributes.size());

        // Get the window from the renderer to determine the color target format
        auto* renderer = Renderer::instance();
        if (!renderer || !renderer->IsInitialized())
        {
            std::cerr << "[PipelineFactory] Renderer not available or not initialized" << std::endl;
            return nullptr;
        }

        // Check if this is a shadow shader - shadow shaders need depth-only pipelines
        bool isShadowShader = (shader->name() == "shaders/shadow");
        
        SDL_GPUColorTargetDescription colorTargetDesc = {};
        if (!isShadowShader)
        {
            colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(device, renderer->GetWindow());
        }

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.vertex_shader = shader->getVertexShader();
        pipelineCreateInfo.fragment_shader = shader->getFragmentShader();
        pipelineCreateInfo.vertex_input_state = vertexInputState;
        pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    
        // Set rasterizer state based on shader properties
        pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        pipelineCreateInfo.rasterizer_state.cull_mode = shader->cullMode();
        pipelineCreateInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        //pipelineCreateInfo.rasterizer_state.enable_depth_clip = false;
        //pipelineCreateInfo.rasterizer_state.enable_depth_bias = false;
        //pipelineCreateInfo.rasterizer_state.depth_bias_constant_factor = 0.0f;
        //pipelineCreateInfo.rasterizer_state.depth_bias_clamp = 0.0f;
        //pipelineCreateInfo.rasterizer_state.depth_bias_slope_factor = 0.0f;
    
        // Set multisample state based on current render target
        // Check if MSAA is active in the renderer
        bool useMSAA = renderer->isMSAAActive();
        pipelineCreateInfo.multisample_state.sample_count = useMSAA ? SDL_GPU_SAMPLECOUNT_4 : SDL_GPU_SAMPLECOUNT_1;
        pipelineCreateInfo.multisample_state.sample_mask = 0;
        pipelineCreateInfo.multisample_state.enable_mask = false;
    
        // Get pipeline properties from shader
        bool depthTest = shader->depthTest();
        bool depthWrite = shader->depthWrite();
        
        pipelineCreateInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        pipelineCreateInfo.depth_stencil_state.back_stencil_state.fail_op = SDL_GPU_STENCILOP_KEEP;
        pipelineCreateInfo.depth_stencil_state.back_stencil_state.pass_op = SDL_GPU_STENCILOP_KEEP;
        pipelineCreateInfo.depth_stencil_state.back_stencil_state.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
        pipelineCreateInfo.depth_stencil_state.back_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        pipelineCreateInfo.depth_stencil_state.front_stencil_state.fail_op = SDL_GPU_STENCILOP_KEEP;
        pipelineCreateInfo.depth_stencil_state.front_stencil_state.pass_op = SDL_GPU_STENCILOP_KEEP;
        pipelineCreateInfo.depth_stencil_state.front_stencil_state.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
        pipelineCreateInfo.depth_stencil_state.front_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        pipelineCreateInfo.depth_stencil_state.compare_mask = 0;
        pipelineCreateInfo.depth_stencil_state.write_mask = 0;
        pipelineCreateInfo.depth_stencil_state.enable_depth_test = depthTest;
        pipelineCreateInfo.depth_stencil_state.enable_depth_write = depthWrite;
        pipelineCreateInfo.depth_stencil_state.enable_stencil_test = false;
    
        // Configure color targets and blend state (only for non-shadow shaders)
        if (!isShadowShader)
        {
            SDL_GPUColorTargetBlendState blendState = {};
            if (shader->isBlendEnabled())
            {
                blendState.enable_blend = true;
                blendState.src_color_blendfactor = shader->srcBlendFactor();
                blendState.dst_color_blendfactor = shader->dstBlendFactor();
                blendState.src_alpha_blendfactor = shader->srcBlendFactor();
                blendState.dst_alpha_blendfactor = shader->dstBlendFactor();
                blendState.color_blend_op = SDL_GPU_BLENDOP_ADD;
                blendState.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
            }
            else
            {
                blendState.enable_blend = false;
            }
            
            // Update color target description with blend state
            colorTargetDesc.blend_state = blendState;
            
            pipelineCreateInfo.target_info.num_color_targets = 1;
            pipelineCreateInfo.target_info.color_target_descriptions = &colorTargetDesc;
        }
        else
        {
            // Shadow shaders are depth-only, no color targets
            pipelineCreateInfo.target_info.num_color_targets = 0;
            pipelineCreateInfo.target_info.color_target_descriptions = nullptr;
        }

        // Set depth stencil target info to match the depth texture
        pipelineCreateInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        pipelineCreateInfo.target_info.has_depth_stencil_target = true;
        
        SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo);
        if (!pipeline)
        {
            std::cerr << "[PipelineFactory] Failed to create graphics pipeline: " << SDL_GetError() << std::endl;
            return nullptr;
        }
        
        return pipeline;
    }

    Uint32 PipelineFactory::getVertexStride(const std::vector<SDL_GPUVertexAttribute>& attributes)
    {
        if (attributes.empty())
            return 0;

        // Calculate stride based on the last attribute's offset + size
        const auto& lastAttr = attributes.back();
        Uint32 stride = lastAttr.offset;
        
        // Add size based on attribute format
        switch (lastAttr.format) {
            case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3:
                stride += 12; // 3 * 4 bytes
                break;
            case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2:
                stride += 8;  // 2 * 4 bytes
                break;
            case SDL_GPU_VERTEXELEMENTFORMAT_INT4:
                stride += 16; // 4 * 4 bytes
                break;
            case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4:
                stride += 16; // 4 * 4 bytes
                break;
            case SDL_GPU_VERTEXELEMENTFORMAT_INT:
                stride += 4;  // 1 * 4 bytes
                break;
            case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT:
                stride += 4;  // 1 * 4 bytes
                break;
            default:
                stride += 4;  // Default to 4 bytes
                break;
        }

        return stride;
    }

    void PipelineFactory::ClearCache()
    {
        for (auto& [key, pipeline] : pipelineCache) {
            if (pipeline) {
                // Note: We need the GPU device to release pipelines
                // For now, just clear the cache entries
            }
        }
        pipelineCache.clear();
    }

} // namespace noz::renderer 