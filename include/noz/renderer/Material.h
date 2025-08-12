/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::renderer
{
    class Shader;

    class Material : public noz::Asset
    {
    public:

        NOZ_DECLARE_TYPEID(Material, noz::Asset)

        virtual ~Material() = default;

        const std::shared_ptr<Shader>& shader() const;

        void setVertexUniformData(uint32_t bufferIndex, const void* data, uint32_t size);
        void setFragmentUniformData(uint32_t bufferIndex, const void* data, uint32_t size);
		void setTexture(const std::shared_ptr<Texture>& texture, uint32_t index = 0);

        std::shared_ptr<Texture> texture(uint32_t index = 0) const;

        size_t textureCount() const;

    private:

        friend class AssetDatabase;
        friend class Renderer;

        struct UniformBufferInfo
        {
            uint32_t size;
            uint32_t offset;
        };

        Material();

        void initialize(const std::shared_ptr<Shader>& shader, const std::string& name);
        void initialize(const std::shared_ptr<Shader>& shader);
        void initialize(const std::string& shaderName, const std::string& name);
        void initialize(const std::string& shaderName);

        const uint8_t* vertexUniformData() const;        
		uint32_t vertexUniformDataSize() const;
		uint32_t vertexUniformBufferCount() const;
        const uint8_t* fragmentUniformData() const;
		uint32_t fragmentUniformDataSize() const;
		uint32_t fragmentUniformBufferCount() const;

        void setUniformBufferData(
            uint32_t bufferIndex,
			const void* data,
            uint32_t size);

        std::shared_ptr<Shader> _shader;
		std::vector<std::shared_ptr<Texture>> _textures;
        std::vector<UniformBufferInfo> _uniformBuffers;
        std::vector<uint8_t> _uniformBufferData;
    };

    inline const std::shared_ptr<Shader>& Material::shader() const { return _shader; }

    inline std::shared_ptr<Texture> Material::texture(uint32_t index) const
    {
        return _textures[index];
    }

    inline size_t Material::textureCount() const
    {
        return _textures.size();
    }
}