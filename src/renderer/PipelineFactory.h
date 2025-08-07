#pragma once

struct SDL_GPUDevice;
struct SDL_GPUGraphicsPipeline;

namespace noz::renderer
{
    class Shader;
    class Mesh;

    class PipelineFactory
    {
    public:
        static SDL_GPUGraphicsPipeline* getOrCreatePipeline(SDL_GPUDevice* device, const std::shared_ptr<Shader>& shader);
        
        static void ClearCache();
        
    private:
        
        static std::unordered_map<size_t, SDL_GPUGraphicsPipeline*> pipelineCache;
        static size_t generatePipelineKey(const std::shared_ptr<Shader>& shader);
        static SDL_GPUGraphicsPipeline* createGraphicsPipeline(SDL_GPUDevice* device, const std::shared_ptr<Shader>& shader, const std::vector<SDL_GPUVertexAttribute>& attributes);
        static Uint32 getVertexStride(const std::vector<SDL_GPUVertexAttribute>& attributes);
    };
} // namespace noz::renderer 