/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::renderer
{
	NOZ_DEFINE_TYPEID(Shader)

    Shader::Shader() 
        : _vertexShader(nullptr)
        , _fragmentShader(nullptr)
        , _srcBlendFactor(SDL_GPU_BLENDFACTOR_ONE)
        , _dstBlendFactor(SDL_GPU_BLENDFACTOR_ZERO)
        , _cullMode(SDL_GPU_CULLMODE_NONE)
		, _flags(Flags::None),
        _vertexUniformBufferCount(0),
        _fragmentUniformBufferCount(0),
        _samplerCount(0)
    {
    }

    Shader::~Shader()
    {
        destroy();
    }

    std::shared_ptr<Shader> Shader::load(const std::string& name)
    {
        auto shader = Object::create<Shader>(name);
        shader->loadInternal();
        return shader;
    }

	void Shader::loadInternal()
    {
        assert(Renderer::instance());
        assert(Renderer::instance()->device());
        auto device = Renderer::instance()->device();

        noz::StreamReader reader;
        if (!reader.loadFromFile(AssetDatabase::getFullPath(name(), "shader")))
            throw std::runtime_error("failed to load file");

        // Read and verify magic
        if (!reader.readFileSignature("SHDR"))
			throw std::runtime_error("invalid shader file format");

        // Read version
        uint32_t version = reader.readUInt32();
        if (version != 1)
			throw std::runtime_error("unsupported shader version: " + std::to_string(version));

        // Read vertex shader bytecode
        std::vector<uint8_t> vertexBytecode = reader.readBytes();
        
        // Read fragment shader bytecode
        std::vector<uint8_t> fragmentBytecode = reader.readBytes();
        
        // Read resource counts
        auto vertexUniformBuffers = reader.readInt32();
        auto fragmentUniformBuffers = reader.readInt32();
        
        // Store uniform buffer counts for Material system
        _vertexUniformBufferCount = vertexUniformBuffers;
        _fragmentUniformBufferCount = fragmentUniformBuffers;
        _samplerCount = reader.readInt32();
        
        // Read pipeline properties
        _flags = static_cast<Flags>(reader.readUInt8());
		_srcBlendFactor = (SDL_GPUBlendFactor)reader.readUInt32();
		_dstBlendFactor = (SDL_GPUBlendFactor)reader.readUInt32();
		_cullMode = (SDL_GPUCullMode)reader.readUInt32();
        
        // Create fragment shader
        SDL_GPUShaderCreateInfo fragmentShaderCreateInfo = {};
        fragmentShaderCreateInfo.code = fragmentBytecode.data();
        fragmentShaderCreateInfo.code_size = fragmentBytecode.size();
        fragmentShaderCreateInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        fragmentShaderCreateInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
        fragmentShaderCreateInfo.entrypoint = "ps";
        fragmentShaderCreateInfo.num_samplers = _samplerCount + static_cast<uint32_t>(registers::Sampler::User0);
        fragmentShaderCreateInfo.num_storage_textures = 0;
        fragmentShaderCreateInfo.num_storage_buffers = 0;
        fragmentShaderCreateInfo.num_uniform_buffers = _fragmentUniformBufferCount + static_cast<uint32_t>(renderer::registers::Fragment::User0);
        fragmentShaderCreateInfo.props = 0;

        _fragmentShader = SDL_CreateGPUShader(device, &fragmentShaderCreateInfo);

        SDL_GPUShaderCreateInfo vertexShaderCreateInfo = {};
        vertexShaderCreateInfo.code = vertexBytecode.data();
        vertexShaderCreateInfo.code_size = vertexBytecode.size();
        vertexShaderCreateInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
        vertexShaderCreateInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
        vertexShaderCreateInfo.entrypoint = "vs";
        vertexShaderCreateInfo.num_samplers = 0;
        vertexShaderCreateInfo.num_storage_textures = 0;
        vertexShaderCreateInfo.num_storage_buffers = 0;
        vertexShaderCreateInfo.num_uniform_buffers = _vertexUniformBufferCount + static_cast<uint32_t>(renderer::registers::Vertex::User0);
        vertexShaderCreateInfo.props = 0;

        _vertexShader = SDL_CreateGPUShader(device, &vertexShaderCreateInfo);

        if (!_vertexShader)
			throw std::runtime_error("failed to create vertex shader: " + std::string(SDL_GetError()));

        if (!_fragmentShader)
        {
            SDL_ReleaseGPUShader(device, _vertexShader);
            _vertexShader = nullptr;
            throw std::runtime_error("failed to create fragment shader: " + std::string(SDL_GetError()));
        }
    }

    void Shader::destroy()
    {
        assert(Renderer::instance());
        assert(Renderer::instance()->device());
        auto device = Renderer::instance()->device();

        if (_vertexShader)
        {
            SDL_ReleaseGPUShader(device, _vertexShader);
            _vertexShader = nullptr;
        }
        if (_fragmentShader)
        {
            SDL_ReleaseGPUShader(device, _fragmentShader);
            _fragmentShader = nullptr;
        }
    }
}
