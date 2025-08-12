/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::renderer
{
    CommandBuffer::CommandBuffer() : _shadowPass(false)
    {
        _commands.reserve(INITIAL_COMMAND_CAPACITY);
        _boneData.reserve(INITIAL_BONE_CAPACITY);
    }

    void CommandBuffer::bind(const std::shared_ptr<Material>& material)
    {
        if (!material) return;
        
        _materials.push_back(material);
        ResourceHandle handle = static_cast<ResourceHandle>(_materials.size());
        _commands.emplace_back(CommandType::BindMaterial, BindMaterialData{handle});
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

    void CommandBuffer::bindColor(const glm::vec4& color)
    {
        _commands.emplace_back(CommandType::BindColor, BindColorData{color});
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

    void CommandBuffer::beginOpaquePass(const std::shared_ptr<Texture>& renderTarget, bool clear, Color clearColor, bool useMSAA)
    {
		assert(renderTarget);

        _textures.push_back(renderTarget);
		_renderTargetTexture = static_cast<ResourceHandle>(_textures.size());

        _commands.emplace_back(
			CommandType::BeginOpaquePassWithTarget,
			BeginOpaquePassWithTargetData
			{
				clear,
				static_cast<ResourceHandle>(_textures.size()),
				clearColor,
                useMSAA
			});
    }

    void CommandBuffer::endOpaquePass(bool setOpaqueTexture)
    {
        _commands.emplace_back(CommandType::EndOpaquePass, EmptyData{});

        if (setOpaqueTexture)
            _opaqueTexture = _renderTargetTexture;
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

    void CommandBuffer::reset()
    {
        _commands.clear();
        _textures.clear();
        _materials.clear();
        _meshes.clear();
        _boneData.clear();
		_opaqueTexture = INVALID_HANDLE;
        _renderTargetTexture = INVALID_HANDLE;
    }
}