#pragma once

struct SDL_GPURenderPass;

namespace noz::renderer
{
    class Texture;
    class Shader;
    class Mesh;

    // Command type enumeration
    enum class CommandType : uint8_t
    {
        BindTexture,
        BindTextureWithSampler,
        BindShader,
        BindMesh,
        SetTransform,
        SetCamera,
        SetBones,
        DrawMesh,
        BeginOpaquePass,
        BeginOpaquePassWithTarget,
        EndOpaquePass,
        BeginShadowPass,
        EndShadowPass,
        SetViewport,
        SetScissor,
        BindLight,
        SetColor,
        SetTextOptions,
        SetGridData,
        SetBufferData
    };

    // Resource handle for thread-safe resource references
    using ResourceHandle = uint32_t;
    constexpr ResourceHandle INVALID_HANDLE = 0;

    // Command data structures
    struct BindTextureData
    {
        ResourceHandle textureHandle;
    };

    struct BindTextureWithSamplerData
    {
        ResourceHandle textureHandle;
        SDL_GPUSampler* sampler;
    };

	struct BindDefaultTexture
	{
	};

    struct BindShaderData
    {
        ResourceHandle shaderHandle;
    };

    struct BindMeshData
    {
        ResourceHandle meshHandle;
    };

    struct SetTransformData
    {
        glm::mat4 transform;
    };

    struct SetCameraData
    {
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
        glm::mat4 viewProjectionMatrix;
    };

    struct SetBonesData
    {
        uint32_t boneCount;
        uint32_t boneDataOffset; // Offset into bone data buffer
    };

    struct DrawMeshData
    {
        ResourceHandle meshHandle;
    };

    struct BeginOpaquePassData
    {
        bool clear;
		Color color;
        bool useMSAA = false;
    };

    struct BeginOpaquePassWithTargetData
    {
        bool clear;
        ResourceHandle renderTargetHandle;
		Color color;
    };

    struct BeginShadowPassData
    {
    };

    struct SetViewportData
    {
        int x, y, width, height;
    };

    struct SetScissorData
    {
        int x, y, width, height;
    };
    
    struct BindLightData
    {
		glm::vec3 ambientColor;
		float ambientIntensity;
		glm::vec3 diffuseColor;
		float diffuseIntensity;
		glm::vec3 lightDirection;
		float shadowBias;
    };
    
    struct SetColorData
    {
        glm::vec4 color;
    };
    
    struct SetTextOptionsData
    {
        glm::vec4 textColor;
        glm::vec4 outlineColor;
        float outlineWidth;
        float smoothing;
    };
    
    struct SetGridDataData
    {
        glm::vec2 gridScale;
        glm::vec2 gridOffset;
    };
    
    struct SetBufferDataData
    {
        uint32_t bufferIndex;  // Which buffer slot (b0, b1, b2, etc.)
        uint32_t size;         // Size of data in bytes
        uint32_t dataOffset;   // Offset into buffer data storage
    };
    
    // Empty struct for commands with no data
    struct EmptyData {};

    // Unified command structure using variant for type safety and memory efficiency
    struct Command
    {
        CommandType type;
        std::variant<
            EmptyData,              // For commands with no data
            BindTextureData,
            BindTextureWithSamplerData,
			BindDefaultTexture,
            BindShaderData,
            BindMeshData,
            SetTransformData,
            SetCameraData,
            SetBonesData,
            DrawMeshData,
            BeginOpaquePassData,
            BeginOpaquePassWithTargetData,
            BeginShadowPassData,
            SetViewportData,
            SetScissorData,
            BindLightData,
            SetColorData,
            SetTextOptionsData,
            SetGridDataData,
            SetBufferDataData
        > data;

        // Default constructor for simple commands with no data
        explicit Command(CommandType t) : type(t), data(EmptyData{}) {}

        // Template constructor for commands with data
        template<typename T>
        Command(CommandType t, const T& d) : type(t), data(d) {}
    };

    class CommandBuffer
    {
    public:
        CommandBuffer();
        ~CommandBuffer() = default;

        // Resource binding commands
        void bind(const std::shared_ptr<Texture>& texture);
        void bindTextureWithSampler(const std::shared_ptr<Texture>& texture, SDL_GPUSampler* sampler);
        void bindShader(const std::shared_ptr<Shader>& shader);
        void bind(const std::shared_ptr<Shader>& shader);
        void bind(const std::shared_ptr<Mesh>& mesh);

		void bindDefaultTexture();
        
        // State setting commands
        void setTransform(const glm::mat4& transform);
        void setCamera(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::mat4& viewProjectionMatrix);
        void setViewProjection(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
        void setBones(const std::vector<glm::mat4>& bones);
        void bindLight(const glm::vec3& lightDirection, float ambientIntensity, const glm::vec3& ambientColor, 
                      float diffuseIntensity, const glm::vec3& diffuseColor, float shadowBias = 0.001f);
        void setColor(const glm::vec4& color);
        void setTextOptions(const glm::vec4& textColor, const glm::vec4& outlineColor = glm::vec4(0.0f), 
                           float outlineWidth = 0.0f, float smoothing = 0.1f);
        void setGridData(const glm::vec2& gridScale, const glm::vec2& gridOffset);
        void setBufferData(uint32_t bufferIndex, const void* data, uint32_t size);
        
        // Drawing commands
        void drawMesh(const std::shared_ptr<Mesh>& mesh);
        
        // Render pass commands
        void beginOpaquePass(bool clear = true, Color clearColor=Color::Black, bool useMSAA = false);
        void beginOpaquePass(const std::shared_ptr<Texture>& renderTarget, bool clear = true, Color clearColor = Color::Black);
        void endOpaquePass();
        void beginShadowPass(const glm::mat4& lightViewMatrix, const glm::mat4& lightProjectionMatrix);
        void endShadowPass();
        
        // Viewport and scissor commands
        void setViewport(int x, int y, int width, int height);
        void setScissor(int x, int y, int width, int height);

        // Internal methods for renderer execution
        const std::vector<Command>& getCommands() const { return _commands; }
        
        // Resource access methods for renderer execution
        std::shared_ptr<Texture> getTexture(ResourceHandle handle) const;
        std::shared_ptr<Shader> getShader(ResourceHandle handle) const;
        std::shared_ptr<Mesh> getMesh(ResourceHandle handle) const;
        const glm::mat4* getBones(uint32_t offset, uint32_t count) const;
        const uint8_t* getBufferData(uint32_t offset, uint32_t size) const;
        
        // Reset for next frame
        void reset();
        
        // Get command count for debugging/profiling
        size_t getCommandCount() const { return _commands.size(); }

		bool isShadowPass() const { return _shadowPass; }

    private:

		std::vector<Command> _commands;
		bool _shadowPass;
        
        // Resource storage
        std::vector<std::shared_ptr<Texture>> _textures;
        std::vector<std::shared_ptr<Shader>> _shaders;
        std::vector<std::shared_ptr<Mesh>> _meshes;
        std::vector<glm::mat4> _boneData;
        std::vector<uint8_t> _bufferData;  // Generic buffer data storage

		int _lastTextureIndex = -1;
        
        // Reserve initial capacity to avoid frequent reallocations
        static constexpr size_t INITIAL_COMMAND_CAPACITY = 1024;
        static constexpr size_t INITIAL_BONE_CAPACITY = 256;
    };

} // namespace noz::renderer