/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

struct SDL_GPUDevice;
struct SDL_GPUBuffer;
struct SDL_GPUTransferBuffer;
struct SDL_GPURenderPass;

namespace noz::renderer
{
	class Skeleton;

    struct Vertex
    {
        glm::vec3 position;
        glm::vec2 uv0;
        glm::vec3 normal;
		glm::vec2 boneIndex;
	};

    class Mesh : public noz::IResource
    {
    public:

        Mesh(const std::string& path);

        ~Mesh();
        
        // Direct data accessors for adding vertex data
        std::vector<glm::vec3>& positions() { return _positions; }
        std::vector<glm::vec3>& normals() { return _normals; }
        std::vector<glm::vec2>& uv0() { return _uv0; }
        std::vector<uint32_t>& boneIndices() { return _boneIndices; }
        std::vector<uint16_t>& indices() { return _indices; }

        // Const accessors for reading data
        const std::vector<glm::vec3>& positions() const { return _positions; }
        const std::vector<glm::vec3>& normals() const { return _normals; }
        const std::vector<glm::vec2>& uv0() const { return _uv0; }
        const std::vector<uint32_t>& boneIndices() const { return _boneIndices; }
        const std::vector<uint16_t>& indices() const { return _indices; }
        
        // Get vertex format (determined automatically during upload)
                
        virtual bool upload(bool clearCpuMemory = false); // Create GPU buffers and upload data
        
        void destroy();
        
        virtual void bind(SDL_GPURenderPass* renderPass) const;
        virtual void draw(SDL_GPURenderPass* renderPass) const;
        
        // Getters
        bool hasIndices() const;
        bool isUploaded() const { return _vertexBuffer != nullptr; }
                        
        size_t getVertexSize() const
        {
			return sizeof(Vertex);
        }

        size_t vertexCount() const;
        size_t indexCount() const;
        
        // Bounds support
        const noz::bounds3& bounds();

        void updateBounds();
                
        // Static methods
        static std::shared_ptr<Mesh> load(const std::string& name);
        static std::shared_ptr<Mesh> loadMeshFile(const std::string& filePath, const std::string& resourceName);
        
    protected:
        
        // Protected members for inheritance
        SDL_GPUDevice* _gpu;
        SDL_GPUBuffer* _vertexBuffer;
        SDL_GPUTransferBuffer* _transferBuffer;
        SDL_GPUBuffer* _indexBuffer;
        SDL_GPUTransferBuffer* _indexTransferBuffer;
        size_t _vertexCount;
        size_t _indexCount;
        
        // CPU-side data storage in separate buffers
        std::vector<glm::vec3> _positions;
        std::vector<glm::vec3> _normals;
        std::vector<glm::vec2> _uv0;
        std::vector<uint32_t> _boneIndices; // Simplified: just bone indices
        std::vector<uint16_t> _indices;
        
        // Bounds (lazy-loaded)
        mutable noz::bounds3 _bounds;
        mutable bool _boundsCalculated = false;
    };

    inline bool Mesh::hasIndices() const 
    {
        return indexCount() > 0;
    }

    inline size_t Mesh::indexCount() const 
    {
        return std::max(_indices.size(), _indexCount);
    }

    inline size_t Mesh::vertexCount() const 
    {
        return std::max(_positions.size(), _vertexCount);
    }

} // namespace noz::renderer
