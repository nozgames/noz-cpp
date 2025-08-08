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
        Shader(const std::string& path);
        ~Shader();
        void destroy();
        SDL_GPUShader* getVertexShader() const { return _vertexShader; }
        SDL_GPUShader* getFragmentShader() const { return _fragmentShader; }
        bool isValid() const { return _vertexShader != nullptr && _fragmentShader != nullptr; }
        
        // Pipeline properties
        bool depthTest() const { return _depthTest; }
        bool depthWrite() const { return _depthWrite; }
        bool isBlendEnabled() const { return _blendEnabled; }
        SDL_GPUBlendFactor srcBlendFactor() const { return _srcBlendFactor; }
        SDL_GPUBlendFactor dstBlendFactor() const { return _dstBlendFactor; }
        SDL_GPUCullMode cullMode() const { return _cullMode; }
        
        // Binding (will be handled by PipelineFactory)
        void bind(SDL_GPURenderPass* renderPass) const;
        
    private:

        friend class AssetDatabase;

        static std::shared_ptr<Shader> load(const std::string& path);
        bool loadInternal(SDL_GPUDevice* gpu, const std::string& name);
        bool loadFromProcessedFile(const std::string& path);

        SDL_GPUDevice* _gpu;
        SDL_GPUShader* _vertexShader;
        SDL_GPUShader* _fragmentShader;
        
        // Pipeline properties loaded from meta file
        bool _depthTest = true;
        bool _depthWrite = true;
        bool _blendEnabled = false;
        SDL_GPUBlendFactor _srcBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
        SDL_GPUBlendFactor _dstBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;
        SDL_GPUCullMode _cullMode = SDL_GPU_CULLMODE_NONE;
    };

} // namespace noz::renderer