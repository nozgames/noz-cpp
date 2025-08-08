/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::renderer
{
    Shader::Shader(const std::string& path)
        : noz::Asset(path)
        , _gpu(nullptr)
        , _vertexShader(nullptr)
        , _fragmentShader(nullptr)
    {
    }

    Shader::~Shader()
    {
        destroy();
    }

    std::shared_ptr<Shader> Shader::load(const std::string& name)
    {
        auto* renderer = Renderer::instance();
        if (!renderer)
            return nullptr;

        auto* gpu = renderer->GetGPUDevice();
        if (!gpu)
            return nullptr;
    
        auto* shader = new Shader(name);
        
        // Load from processed shader format
        if (shader->loadInternal(gpu, name))
            return std::shared_ptr<Shader>(shader);
        
        delete shader;
        return nullptr;
    }

	bool Shader::loadInternal(SDL_GPUDevice* gpu, const std::string& shaderName)
    {
        if (!gpu)
        {
            std::cerr << "Invalid GPU device" << std::endl;
            return false;
        }

        _gpu = gpu;
        
        auto path = AssetDatabase::getFullPath(name(), "shader");

        if (!loadFromProcessedFile(path))
        {
            std::cerr << "Failed to load shader from processed file: " << path << std::endl;
            return false;
        }

        return true;
    }

    bool Shader::loadFromProcessedFile(const std::string& path)
    {
        noz::StreamReader reader;
        if (!reader.loadFromFile(path))
        {
            std::cerr << "Failed to open processed shader file: " << path << std::endl;
            return false;
        }

        // Read and verify magic
        if (!reader.readFileSignature("SHDR"))
        {
            std::cerr << "Invalid shader file format: " << path << std::endl;
            return false;
        }

        // Read version
        uint32_t version = reader.readUInt32();
        if (version != 1)
        {
            std::cerr << "Unsupported shader version: " << version << std::endl;
            return false;
        }

        // Read vertex shader bytecode
        std::vector<uint8_t> vertexBytecode = reader.readBytes();
        
        // Read fragment shader bytecode
        std::vector<uint8_t> fragmentBytecode = reader.readBytes();
        
        // Read resource counts
        int vertexUniformBuffers = reader.readInt32();
        int fragmentUniformBuffers = reader.readInt32();
        int samplers = reader.readInt32();
        int storageTextures = reader.readInt32();
        int storageBuffers = reader.readInt32();
        
        // Read pipeline properties
		_depthTest = reader.readBool();
		_depthWrite = reader.readBool();
		_blendEnabled = reader.readBool();        		
		_srcBlendFactor = (SDL_GPUBlendFactor)reader.readUInt32();
		_dstBlendFactor = (SDL_GPUBlendFactor)reader.readUInt32();
		_cullMode = (SDL_GPUCullMode)reader.readUInt32();
        
        // Create shader objects from bytecode
        if (!_gpu)
        {
            std::cerr << "No GPU device available for shader compilation" << std::endl;
            return false;
        }

        // Create fragment shader
        SDL_GPUShaderCreateInfo fragmentShaderCreateInfo = {};
        fragmentShaderCreateInfo.code = fragmentBytecode.data();
        fragmentShaderCreateInfo.code_size = fragmentBytecode.size();
        fragmentShaderCreateInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        fragmentShaderCreateInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
        fragmentShaderCreateInfo.entrypoint = "ps";
        fragmentShaderCreateInfo.num_samplers = samplers;
        fragmentShaderCreateInfo.num_storage_textures = storageTextures;
        fragmentShaderCreateInfo.num_storage_buffers = storageBuffers;
        fragmentShaderCreateInfo.num_uniform_buffers = fragmentUniformBuffers;
        fragmentShaderCreateInfo.props = 0;

        _fragmentShader = SDL_CreateGPUShader(_gpu, &fragmentShaderCreateInfo);

        SDL_GPUShaderCreateInfo vertexShaderCreateInfo = {};
        vertexShaderCreateInfo.code = vertexBytecode.data();
        vertexShaderCreateInfo.code_size = vertexBytecode.size();
        vertexShaderCreateInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
        vertexShaderCreateInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
        vertexShaderCreateInfo.entrypoint = "vs";
        vertexShaderCreateInfo.num_samplers = 0; // Vertex shaders don't use samplers
        vertexShaderCreateInfo.num_storage_textures = 0; // For now, assume no vertex storage textures
        vertexShaderCreateInfo.num_storage_buffers = 0; // For now, assume no vertex storage buffers
        vertexShaderCreateInfo.num_uniform_buffers = vertexUniformBuffers;
        vertexShaderCreateInfo.props = 0;

        _vertexShader = SDL_CreateGPUShader(_gpu, &vertexShaderCreateInfo);

        if (!_vertexShader)
        {
            std::cerr << "Failed to create vertex shader: " << SDL_GetError() << std::endl;
            return false;
        }

        if (!_fragmentShader)
        {
            std::cerr << "Failed to create fragment shader: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUShader(_gpu, _vertexShader);
            _vertexShader = nullptr;
            return false;
        }

        return true;
    }

    void Shader::bind(SDL_GPURenderPass* renderPass) const
    {
    }

    void Shader::destroy()
    {
        if (_vertexShader)
        {
            SDL_ReleaseGPUShader(_gpu, _vertexShader);
            _vertexShader = nullptr;
        }
        if (_fragmentShader)
        {
            SDL_ReleaseGPUShader(_gpu, _fragmentShader);
            _fragmentShader = nullptr;
        }
        _gpu = nullptr;
    }

} // namespace noz::renderer 