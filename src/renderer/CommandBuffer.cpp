/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::renderer
{
    // CommandBuffer implementation
    CommandBuffer::CommandBuffer() : _shadowPass(false)
    {
        _commands.reserve(INITIAL_COMMAND_CAPACITY);
        _boneData.reserve(INITIAL_BONE_CAPACITY);
    }

    void CommandBuffer::bind(const std::shared_ptr<Texture>& texture)
    {
		if (!texture)
		{
			bindDefaultTexture();
			return;
		}

		if (_lastTextureIndex > 0 && _textures[_lastTextureIndex].get() == texture.get())
			return;
        
        _textures.push_back(texture);
        ResourceHandle handle = static_cast<ResourceHandle>(_textures.size());
        _commands.emplace_back(CommandType::BindTexture, BindTextureData{handle});
		_lastTextureIndex = static_cast<int>(_textures.size()) - 1;
    }

    void CommandBuffer::bindTextureWithSampler(const std::shared_ptr<Texture>& texture, SDL_GPUSampler* sampler)
    {
		if (!texture || !sampler)
		{
			bindDefaultTexture();
			return;
		}
        
        _textures.push_back(texture);
        ResourceHandle handle = static_cast<ResourceHandle>(_textures.size());
        _commands.emplace_back(CommandType::BindTextureWithSampler, BindTextureWithSamplerData{handle, sampler});
		_lastTextureIndex = static_cast<int>(_textures.size()) - 1;
    }

	void CommandBuffer::bindDefaultTexture()
	{
		bind(Renderer::instance()->defaultTexture());
	}

    void CommandBuffer::bindShader(const std::shared_ptr<Shader>& shader)
    {
        bind(shader);
    }

    void CommandBuffer::bind(const std::shared_ptr<Shader>& shader)
    {
        if (!shader) return;
        
        _shaders.push_back(shader);
        ResourceHandle handle = static_cast<ResourceHandle>(_shaders.size());
        _commands.emplace_back(CommandType::BindShader, BindShaderData{handle});
    }

    void CommandBuffer::bind(const std::shared_ptr<Mesh>& mesh)
    {
        if (!mesh) return;
        
        _meshes.push_back(mesh);
        ResourceHandle handle = static_cast<ResourceHandle>(_meshes.size());
        _commands.emplace_back(CommandType::BindMesh, BindMeshData{handle});
    }

    void CommandBuffer::setTransform(const glm::mat4& transform)
    {
        _commands.emplace_back(CommandType::SetTransform, SetTransformData{transform});
    }

    void CommandBuffer::setCamera(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::mat4& viewProjectionMatrix)
    {
        SetCameraData data;
        data.viewMatrix = viewMatrix;
        data.projectionMatrix = projectionMatrix;
        data.viewProjectionMatrix = viewProjectionMatrix;
        _commands.emplace_back(CommandType::SetCamera, data);
    }

    void CommandBuffer::setViewProjection(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
    {
        SetCameraData data;
        data.viewMatrix = viewMatrix;
        data.projectionMatrix = projectionMatrix;
        data.viewProjectionMatrix = projectionMatrix * viewMatrix;
        _commands.emplace_back(CommandType::SetCamera, data);
    }

    void CommandBuffer::setBones(const std::vector<glm::mat4>& bones)
    {
        if (bones.empty()) return;
        
        uint32_t offset = static_cast<uint32_t>(_boneData.size());
        _boneData.insert(_boneData.end(), bones.begin(), bones.end());
        
        SetBonesData data;
        data.boneCount = static_cast<uint32_t>(bones.size());
        data.boneDataOffset = offset;
        _commands.emplace_back(CommandType::SetBones, data);
    }

    void CommandBuffer::bindLight(const glm::vec3& lightDirection, float ambientIntensity, const glm::vec3& ambientColor, 
                                 float diffuseIntensity, const glm::vec3& diffuseColor, float shadowBias)
    {
        BindLightData data;
        data.lightDirection = lightDirection;
        data.ambientIntensity = ambientIntensity;
        data.ambientColor = ambientColor;
        data.diffuseIntensity = diffuseIntensity;
        data.diffuseColor = diffuseColor;
        data.shadowBias = shadowBias;
        _commands.emplace_back(CommandType::BindLight, data);
    }

    void CommandBuffer::setColor(const glm::vec4& color)
    {
        _commands.emplace_back(CommandType::SetColor, SetColorData{color});
    }

    void CommandBuffer::setTextOptions(const glm::vec4& textColor, const glm::vec4& outlineColor, 
                                      float outlineWidth, float smoothing)
    {
        SetTextOptionsData data;
        data.textColor = textColor;
        data.outlineColor = outlineColor;
        data.outlineWidth = outlineWidth;
        data.smoothing = smoothing;
        _commands.emplace_back(CommandType::SetTextOptions, data);
    }
    
    void CommandBuffer::setGridData(const glm::vec2& gridScale, const glm::vec2& gridOffset)
    {
        SetGridDataData data;
        data.gridScale = gridScale;
        data.gridOffset = gridOffset;
        _commands.emplace_back(CommandType::SetGridData, data);
    }

    void CommandBuffer::drawMesh(const std::shared_ptr<Mesh>& mesh)
    {
        if (!mesh) return;
        
        _meshes.push_back(mesh);
        ResourceHandle handle = static_cast<ResourceHandle>(_meshes.size());
        _commands.emplace_back(CommandType::DrawMesh, DrawMeshData{handle});
    }

    void CommandBuffer::beginOpaquePass(bool clear, Color clearColor, bool useMSAA)
    {
        _commands.emplace_back(CommandType::BeginOpaquePass, BeginOpaquePassData{clear, clearColor, useMSAA });
    }

    void CommandBuffer::beginOpaquePass(const std::shared_ptr<Texture>& renderTarget, bool clear, Color clearColor)
    {
		assert(renderTarget);

        _textures.push_back(renderTarget);
        _commands.emplace_back(
			CommandType::BeginOpaquePassWithTarget,
			BeginOpaquePassWithTargetData
			{
				clear,
				static_cast<ResourceHandle>(_textures.size()),
				clearColor
			});
    }

    void CommandBuffer::endOpaquePass()
    {
        _commands.emplace_back(CommandType::EndOpaquePass, EmptyData{});
		_lastTextureIndex = -1;
    }

    void CommandBuffer::beginShadowPass(const glm::mat4& lightViewMatrix, const glm::mat4& lightProjectionMatrix)
    {
		_commands.emplace_back(CommandType::BeginShadowPass, EmptyData{});
		_shadowPass = true;
		setCamera(lightViewMatrix, lightProjectionMatrix, lightProjectionMatrix * lightViewMatrix);
    }

    void CommandBuffer::endShadowPass()
    {
        _commands.emplace_back(CommandType::EndShadowPass, EmptyData{});
		_lastTextureIndex = -1;
		_shadowPass = false;
    }

    void CommandBuffer::setViewport(int x, int y, int width, int height)
    {
        _commands.emplace_back(CommandType::SetViewport, SetViewportData{x, y, width, height});
    }

    void CommandBuffer::setScissor(int x, int y, int width, int height)
    {
        _commands.emplace_back(CommandType::SetScissor, SetScissorData{x, y, width, height});
    }

    std::shared_ptr<Texture> CommandBuffer::getTexture(ResourceHandle handle) const
    {
        if (handle == INVALID_HANDLE || handle > _textures.size()) return nullptr;
        return _textures[handle - 1]; // Handle is 1-based
    }

    std::shared_ptr<Shader> CommandBuffer::getShader(ResourceHandle handle) const
    {
        if (handle == INVALID_HANDLE || handle > _shaders.size()) return nullptr;
        return _shaders[handle - 1]; // Handle is 1-based
    }

    std::shared_ptr<Mesh> CommandBuffer::getMesh(ResourceHandle handle) const
    {
        if (handle == INVALID_HANDLE || handle > _meshes.size()) return nullptr;
        return _meshes[handle - 1]; // Handle is 1-based
    }

    const glm::mat4* CommandBuffer::getBones(uint32_t offset, uint32_t count) const
    {
        if (offset + count > _boneData.size()) return nullptr;
        return &_boneData[offset];
    }

    void CommandBuffer::setBufferData(uint32_t bufferIndex, const void* data, uint32_t size)
    {
        if (!data || size == 0) return;
        
        uint32_t offset = static_cast<uint32_t>(_bufferData.size());
        
        // Align to 16-byte boundary for GPU constant buffers
        uint32_t alignedOffset = (offset + 15) & ~15;
        uint32_t padding = alignedOffset - offset;
        
        // Add padding if needed
        if (padding > 0) {
            _bufferData.resize(_bufferData.size() + padding, 0);
        }
        
        // Copy the data
        size_t currentSize = _bufferData.size();
        _bufferData.resize(currentSize + size);
        memcpy(_bufferData.data() + currentSize, data, size);
        
        // Add command
        _commands.emplace_back(CommandType::SetBufferData, SetBufferDataData{bufferIndex, size, alignedOffset});
    }

    const uint8_t* CommandBuffer::getBufferData(uint32_t offset, uint32_t size) const
    {
        if (offset + size > _bufferData.size()) return nullptr;
        return &_bufferData[offset];
    }

    void CommandBuffer::reset()
    {
        _commands.clear();
        _textures.clear();
        _shaders.clear();
        _meshes.clear();
        _boneData.clear();
        _bufferData.clear();
		_lastTextureIndex = -1;
    }
}