/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::renderer
{
    NOZ_DEFINE_TYPEID(Material)

    Material::Material()        
    {
    }

    void Material::initialize(const std::string& shaderName)
    {
		initialize(Asset::load<Shader>(shaderName));
    }

    void Material::initialize(const std::string& shaderName, const std::string& name)
    {
        initialize(Asset::load<Shader>(shaderName), name);
    }

    void Material::initialize(const std::shared_ptr<Shader>& shader, const std::string& name)
    {
        Asset::initialize(name);

        assert(shader);

        _shader = shader;
        _textures.resize(_shader->samplerCount());
        _uniformBuffers.resize(vertexUniformBufferCount() + fragmentUniformBufferCount());
    }

    void Material::initialize(const std::shared_ptr<Shader>& shader)
    {
		initialize(shader, shader->name());
    }

    void Material::setUniformBufferData(uint32_t bufferIndex, const void* data, uint32_t size)
    {
		assert(bufferIndex < _uniformBuffers.size());
        assert(data);
        assert(size > 0);

		auto& uniformBuffer = _uniformBuffers[bufferIndex];
        auto resize = (int)size - (int)uniformBuffer.size;
        auto afterStart = _uniformBufferData.size() - uniformBuffer.offset - uniformBuffer.size;
		auto afterSize = _uniformBufferData.size() - afterStart;

        uniformBuffer.size = size;

        // Adjust memory after
        if (resize > 0)
        {
            if (afterSize > 0)
                std::memmove(_uniformBufferData.data() + afterStart, _uniformBufferData.data() + afterStart + resize, afterSize);

            for (uint32_t i = bufferIndex + 1; i < _uniformBuffers.size(); ++i)
                _uniformBuffers[i].offset = uint32_t(int(_uniformBuffers[i].offset) - resize);
        }
    }

    void Material::setVertexUniformData(uint32_t bufferIndex, const void* data, uint32_t size)
    {
        assert(bufferIndex < vertexUniformBufferCount());
        assert(data);
		assert(size > 0);

        setUniformBufferData(bufferIndex, data, size);
    }

    void Material::setFragmentUniformData(uint32_t bufferIndex, const void* data, uint32_t size)
    {
		assert(bufferIndex < fragmentUniformBufferCount());
        assert(data);
		assert(size > 0);
        setUniformBufferData(
            bufferIndex + vertexUniformBufferCount(),
			data,
            size);
    }

    void Material::setTexture(const std::shared_ptr<Texture>& texture, uint32_t index)
    {
        assert(index < _textures.size());
		_textures[index] = texture; 
	}

    uint32_t Material::vertexUniformBufferCount() const
    {
        return _shader->vertexUniformBufferCount();
    }

    const uint8_t* Material::vertexUniformData() const
    {
        return _uniformBufferData.data() + _uniformBuffers[0].offset;
    }

    uint32_t Material::vertexUniformDataSize() const
    {
		auto& buffer = _uniformBuffers[vertexUniformBufferCount()-1];
		return buffer.offset + buffer.size;
	}

    uint32_t Material::fragmentUniformBufferCount() const
    {
        return _shader->fragmentUniformBufferCount();
    }

    const uint8_t* Material::fragmentUniformData() const
    {
        assert(fragmentUniformBufferCount() > 0);
        return _uniformBufferData.data() + _uniformBuffers[vertexUniformBufferCount()].offset;
    }

    uint32_t Material::fragmentUniformDataSize() const
    {
        assert(fragmentUniformBufferCount() > 0);
        auto& buffer = _uniformBuffers.back();
		return buffer.offset + buffer.size;
	}
}
