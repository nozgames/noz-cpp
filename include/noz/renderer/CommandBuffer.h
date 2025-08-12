/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

struct SDL_GPURenderPass;

namespace noz::renderer
{
    class Texture;
    class Shader;
    class Material;
    class Mesh;

    // Command type enumeration
    enum class CommandType : uint8_t
    {
        BindMaterial,
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
        SetTextOptions
    };

    // Resource handle for thread-safe resource references
    using ResourceHandle = int;
    constexpr ResourceHandle INVALID_HANDLE = -1;

    struct BindMaterialData
    {
        ResourceHandle material;
    };

    struct BindMeshData
    {
        ResourceHandle mesh;
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
        ResourceHandle mesh;
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
        ResourceHandle renderTarget;
		Color color;
        bool useMSAA;
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
        float padding1;
        float padding2;
    };
        
    // Empty struct for commands with no data
    struct EmptyData {};

    // Unified command structure using variant for type safety and memory efficiency
    struct Command
    {
        CommandType type;
        std::variant<
            EmptyData,              // For commands with no data
            BindMaterialData,
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
            SetTextOptionsData
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

        void bind(const std::shared_ptr<Material>& material);
        void bind(const std::shared_ptr<Mesh>& mesh);
        
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
        
        // Drawing commands
        void drawMesh(const std::shared_ptr<Mesh>& mesh);
        
        // Render pass commands
        void beginOpaquePass(bool clear = true, Color clearColor=Color::Black, bool useMSAA = false);
        void beginOpaquePass(const std::shared_ptr<Texture>& renderTarget, bool clear = true, Color clearColor = Color::Black, bool useMSAA = false);
        void endOpaquePass(bool setOpaqueTexture = false);
        void beginShadowPass(const glm::mat4& lightViewMatrix, const glm::mat4& lightProjectionMatrix);
        void endShadowPass();
        
        // Viewport and scissor commands
        void setViewport(int x, int y, int width, int height);
        void setScissor(int x, int y, int width, int height);

        // Internal methods for renderer execution
        const std::vector<Command>& getCommands() const { return _commands; }
        
        // Resource access methods for renderer execution
        std::shared_ptr<Texture> texture(ResourceHandle handle) const;
        std::shared_ptr<Shader> shader(ResourceHandle handle) const;
        std::shared_ptr<Material> material(ResourceHandle handle) const;
        std::shared_ptr<Mesh> mesh(ResourceHandle handle) const;
        const glm::mat4* bones(uint32_t offset, uint32_t count) const;
        
        // Reset for next frame
        void reset();
        
        // Get command count for debugging/profiling
        size_t getCommandCount() const { return _commands.size(); }

		bool isShadowPass() const { return _shadowPass; }

		std::shared_ptr<Texture> opaqueTexture() const; 

    private:

		std::vector<Command> _commands;
		bool _shadowPass;
        
        std::vector<std::shared_ptr<Texture>> _textures;
        std::vector<std::shared_ptr<Material>> _materials;
        std::vector<std::shared_ptr<Mesh>> _meshes;
        std::vector<glm::mat4> _boneData;

		int _lastTextureIndex = -1;
        ResourceHandle _opaqueTexture;
        ResourceHandle _renderTargetTexture;
        
        // Reserve initial capacity to avoid frequent reallocations
        static constexpr size_t INITIAL_COMMAND_CAPACITY = 1024;
        static constexpr size_t INITIAL_BONE_CAPACITY = 256;
    };

    inline std::shared_ptr<Texture> CommandBuffer::opaqueTexture() const
    {
        if (_opaqueTexture == INVALID_HANDLE) return nullptr;
		return texture(_opaqueTexture);
    }

    inline std::shared_ptr<Texture> CommandBuffer::texture(ResourceHandle handle) const
    {
        if (handle == INVALID_HANDLE || handle > _textures.size()) return nullptr;
        return _textures[handle - 1]; // Handle is 1-based
    }

    inline std::shared_ptr<Material> CommandBuffer::material(ResourceHandle handle) const
    {
        if (handle == INVALID_HANDLE || handle > _materials.size()) return nullptr;
        return _materials[handle - 1]; // Handle is 1-based
    }

    inline std::shared_ptr<Mesh> CommandBuffer::mesh(ResourceHandle handle) const
    {
        if (handle == INVALID_HANDLE || handle > _meshes.size()) return nullptr;
        return _meshes[handle - 1]; // Handle is 1-based
    }

    inline const glm::mat4* CommandBuffer::bones(uint32_t offset, uint32_t count) const
    {
        if (offset + count > _boneData.size()) return nullptr;
        return &_boneData[offset];
    }


}
