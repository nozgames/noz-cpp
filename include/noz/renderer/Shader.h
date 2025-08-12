/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

struct SDL_GPUDevice;
struct SDL_GPUShader;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPURenderPass;

namespace noz::renderer
{
    class Shader : public noz::Asset
    {
    public:

        enum class Flags : uint8_t 
        {
            None = 0,
            DepthTest = 1 << 0,
            DepthWrite = 1 << 1,
            Blend = 1 << 2,
            Light = 1 << 3,
			Shadows = 1 << 4,
		};

		NOZ_DECLARE_TYPEID(Shader, noz::Asset)

        ~Shader();
        void destroy();
        SDL_GPUShader* getVertexShader() const { return _vertexShader; }
        SDL_GPUShader* getFragmentShader() const { return _fragmentShader; }
        bool isValid() const { return _vertexShader != nullptr && _fragmentShader != nullptr; }
        
        bool depthTest() const;
        bool depthWrite() const;
        bool blendEnabled() const;
        SDL_GPUBlendFactor srcBlendFactor() const { return _srcBlendFactor; }
        SDL_GPUBlendFactor dstBlendFactor() const { return _dstBlendFactor; }
        SDL_GPUCullMode cullMode() const { return _cullMode; }
        
        int vertexUniformBufferCount() const;
        int fragmentUniformBufferCount() const;
        int samplerCount() const;
        
        void bind(SDL_GPURenderPass* renderPass) const;
        
    private:

        friend class AssetDatabase;

        Shader();

        static std::shared_ptr<Shader> load(const std::string& path);
        void loadInternal();

        SDL_GPUShader* _vertexShader;
        SDL_GPUShader* _fragmentShader;        
        int _vertexUniformBuffers;
        int _fragmentUniformBuffers;
        int _samplers;        
		Flags _flags;
        SDL_GPUBlendFactor _srcBlendFactor;
        SDL_GPUBlendFactor _dstBlendFactor;
        SDL_GPUCullMode _cullMode;
    };

    inline int Shader::vertexUniformBufferCount() const { return _vertexUniformBuffers; }
    inline int Shader::fragmentUniformBufferCount() const { return _fragmentUniformBuffers; }
    inline int Shader::samplerCount() const { return _samplers; }

    inline bool Shader::depthTest() const
    {
        return (static_cast<uint8_t>(_flags) & static_cast<uint8_t>(Flags::DepthTest)) == static_cast<uint8_t>(Flags::DepthTest);
    }

    inline bool Shader::depthWrite() const
    {
        return (static_cast<uint8_t>(_flags) & static_cast<uint8_t>(Flags::DepthWrite)) == static_cast<uint8_t>(Flags::DepthWrite);
    }

    inline bool Shader::blendEnabled() const
    {
        return (static_cast<uint8_t>(_flags) & static_cast<uint8_t>(Flags::Blend)) == static_cast<uint8_t>(Flags::Blend);
    }

}
