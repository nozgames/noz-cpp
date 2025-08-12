/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/renderer/MeshData.h>

namespace noz::renderer
{
    NOZ_DEFINE_TYPEID(Mesh);

    Mesh::Mesh()
        : _vertexBuffer(nullptr)
        , _transferBuffer(nullptr)
        , _indexBuffer(nullptr)
        , _indexTransferBuffer(nullptr)
        , _vertexCount(0)
        , _indexCount(0)
    {
    }

    Mesh::~Mesh()
    {
        destroy();
    }

    std::shared_ptr<Mesh> Mesh::load(const std::string& name)
    {
        auto mesh = Object::create<Mesh>(name);
        mesh->loadInternal();
        return mesh;
    }

    void Mesh::loadInternal()
    {
        noz::StreamReader reader;
        if (!reader.loadFromFile(AssetDatabase::getFullPath(name(), "mesh")))
            throw std::runtime_error("Invalid stream");

		if (reader.readFileSignature("MESH") == false)
			throw std::runtime_error("Invalid mesh file signature");

        auto gpu = reader.readBool();
        auto cpu = reader.readBool();

        // Read pre-calculated bounds
        glm::vec3 boundsMin, boundsMax;
        boundsMin.x = reader.readFloat();
        boundsMin.y = reader.readFloat();
        boundsMin.z = reader.readFloat();
        boundsMax.x = reader.readFloat();
        boundsMax.y = reader.readFloat();
        boundsMax.z = reader.readFloat();

        // Read model data header
        noz::renderer::ModelData modelData;
        modelData.vertexCount = reader.readUInt16();
        modelData.indexCount = reader.readUInt16();
        modelData.hasNormals = reader.readBool();
        modelData.hasUVs = reader.readBool();
        modelData.hasBoneIndices = reader.readBool();
        modelData.hasAnimations = reader.readBool();
        reader.readUInt8(); // Skip padding[0]
        reader.readUInt8(); // Skip padding[1]
                
        // Read vertex data
        if (modelData.vertexCount > 0)
        {
            positions().resize(modelData.vertexCount);
            for (uint16_t i = 0; i < modelData.vertexCount; ++i)
            {
                positions()[i].x = reader.readFloat();
                positions()[i].y = reader.readFloat();
                positions()[i].z = reader.readFloat();
            }
            
            if (modelData.hasNormals)
            {
                normals().resize(modelData.vertexCount);
                for (uint16_t i = 0; i < modelData.vertexCount; ++i)
                {
                    normals()[i].x = reader.readFloat();
                    normals()[i].y = reader.readFloat();
                    normals()[i].z = reader.readFloat();
                }
            }
            
            if (modelData.hasUVs)
            {
                uv0().resize(modelData.vertexCount);
                for (uint16_t i = 0; i < modelData.vertexCount; ++i)
                {
                    uv0()[i].x = reader.readFloat();
                    uv0()[i].y = reader.readFloat();
                }
            }
            
            if (modelData.hasBoneIndices)
            {
                boneIndices().resize(modelData.vertexCount);
                for (uint16_t i = 0; i < modelData.vertexCount; ++i)
                    boneIndices()[i] = reader.readUInt32();
            }
        }
        
        // Read index data
        if (modelData.indexCount > 0)
        {
            indices().resize(modelData.indexCount);
            for (uint16_t i = 0; i < modelData.indexCount; ++i)
            {
                indices()[i] = reader.readUInt16();
            }
        }

        // Set the pre-calculated bounds
        _bounds = noz::bounds3();
        _bounds.min() = boundsMin;
        _bounds.max() = boundsMax;
        _boundsCalculated = true;

        // Optionally upload the mesh
        if (gpu)
            upload(!cpu);
    }

    bool Mesh::upload(bool clearCpuMemory)
    {
        assert(Renderer::instance());
        assert(Renderer::instance()->device());

        if (_vertexBuffer)
        {
            std::cerr << "Mesh already uploaded" << std::endl;
            return false;
        }

        auto device = Renderer::instance()->device();
        auto vertexCount = _positions.size();
        auto indexCount = _indices.size();

        // Create vertex buffer
        SDL_GPUBufferCreateInfo vbinfo = {};
        vbinfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vbinfo.size = static_cast<Uint32>(sizeof(Vertex) * vertexCount);
        vbinfo.props = 0;
        _vertexBuffer = SDL_CreateGPUBuffer(device, &vbinfo);

        if (!_vertexBuffer)
        {
            std::cerr << "Failed to create vertex buffer: " << SDL_GetError() << std::endl;
            return false;
        }

        // Create transfer buffer
        SDL_GPUTransferBufferCreateInfo tbinfo = {};
        tbinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbinfo.size = vbinfo.size;
        tbinfo.props = 0;
        _transferBuffer = SDL_CreateGPUTransferBuffer(device, &tbinfo);

        if (!_transferBuffer)
        {
            std::cerr << "Failed to create transfer buffer: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUBuffer(device, _vertexBuffer);
            _vertexBuffer = nullptr;
            return false;
        }

        // Upload vertex data
        SDL_GPUTransferBufferLocation source = { _transferBuffer, 0 };
        SDL_GPUBufferRegion destination = {_vertexBuffer, 0, vbinfo.size};
        void* mappedData = SDL_MapGPUTransferBuffer(device, _transferBuffer, false);
        
        if (!mappedData)
        {
            std::cerr << "Failed to map transfer buffer" << std::endl;
            return false;
        }

		std::vector<Vertex> vertices;
		vertices.reserve(vertexCount);
		for (size_t i = 0; i < vertexCount; ++i)
		{
			Vertex vertex;
			vertex.position = _positions[i];
			vertex.normal = i < _normals.size() ? _normals[i] : glm::vec3(0.0f, 1.0f, 0.0f); // Default up normal
			vertex.uv0 = i < _uv0.size() ? _uv0[i] : glm::vec2(0.0f, 0.0f); // Default UV
			vertex.boneIndex = i < _boneIndices.size() ? glm::vec2(static_cast<float>(_boneIndices[i]), 0) : glm::vec2(0.0f, 0.0f); // Default bone index
			vertices.push_back(vertex);
		}

		SDL_memcpy(mappedData, vertices.data(), vbinfo.size);        
        SDL_UnmapGPUTransferBuffer(device, _transferBuffer);
        
        // Upload vertex data to GPU using command buffer
        SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);
        SDL_UploadToGPUBuffer(copyPass, &source, &destination, false);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmd);
        
        // Create index buffer if we have indices
        if (indexCount > 0)
        {
            SDL_GPUBufferCreateInfo ibinfo = {};
            ibinfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
            ibinfo.size = static_cast<Uint32>(sizeof(uint16_t) * indexCount);
            ibinfo.props = 0;
            _indexBuffer = SDL_CreateGPUBuffer(device, &ibinfo);

            if (!_indexBuffer)
            {
                std::cerr << "Failed to create index buffer: " << SDL_GetError() << std::endl;
                return false;
            }

            // Create index transfer buffer
            SDL_GPUTransferBufferCreateInfo itbinfo = {};
            itbinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            itbinfo.size = ibinfo.size;
            itbinfo.props = 0;
            _indexTransferBuffer = SDL_CreateGPUTransferBuffer(device, &itbinfo);

            if (!_indexTransferBuffer)
            {
                std::cerr << "Failed to create index transfer buffer: " << SDL_GetError() << std::endl;
                SDL_ReleaseGPUBuffer(device, _indexBuffer);
                _indexBuffer = nullptr;
                return false;
            }

            // Upload index data
            SDL_GPUTransferBufferLocation indexSource = { _indexTransferBuffer, 0 };
            SDL_GPUBufferRegion indexDestination = {_indexBuffer, 0, ibinfo.size};
            void* indexMappedData = SDL_MapGPUTransferBuffer(device, _indexTransferBuffer, false);
            
            if (!indexMappedData)
            {
                std::cerr << "Failed to map index transfer buffer" << std::endl;
                return false;
            }
            
            SDL_memcpy(indexMappedData, _indices.data(), ibinfo.size);
            SDL_UnmapGPUTransferBuffer(device, _indexTransferBuffer);
            
            // Upload index data to GPU using command buffer
            SDL_GPUCommandBuffer* indexUploadCmd = SDL_AcquireGPUCommandBuffer(device);
            SDL_GPUCopyPass* indexCopyPass = SDL_BeginGPUCopyPass(indexUploadCmd);
            SDL_UploadToGPUBuffer(indexCopyPass, &indexSource, &indexDestination, false);
            SDL_EndGPUCopyPass(indexCopyPass);
            SDL_SubmitGPUCommandBuffer(indexUploadCmd);
        }

        // Store counts
        _vertexCount = vertexCount;
        _indexCount = indexCount;

        // Clear CPU memory if requested
        if (clearCpuMemory)
        {
            _positions.clear();
            _normals.clear();
            _uv0.clear();
            _boneIndices.clear();
            _indices.clear();
            // Keep bones data for animation
        }

        return true;
    }

    void Mesh::destroy()
    {
        // GPU device not set, just clear member variables
        if (!_vertexBuffer)
        {
            _vertexBuffer = nullptr;
            _transferBuffer = nullptr;
            _indexBuffer = nullptr;
            _indexTransferBuffer = nullptr;
            _vertexCount = 0;
            _indexCount = 0;
            return;
        }

        assert(Renderer::instance());
        assert(Renderer::instance()->device());
        auto device = Renderer::instance()->device();

        if (_indexTransferBuffer)
        {
            SDL_ReleaseGPUTransferBuffer(device, _indexTransferBuffer);
            _indexTransferBuffer = nullptr;
        }
        if (_indexBuffer)
        {
            SDL_ReleaseGPUBuffer(device, _indexBuffer);
            _indexBuffer = nullptr;
        }
        if (_transferBuffer)
        {
            SDL_ReleaseGPUTransferBuffer(device, _transferBuffer);
            _transferBuffer = nullptr;
        }
        if (_vertexBuffer)
        {
            SDL_ReleaseGPUBuffer(device, _vertexBuffer);
            _vertexBuffer = nullptr;
        }

        _vertexCount = 0;
        _indexCount = 0;
        
        // Clear separate buffer data
        _positions.clear();
        _normals.clear();
        _uv0.clear();
        _boneIndices.clear();
        _indices.clear();
    }

    void Mesh::bind(SDL_GPURenderPass* renderPass) const 
    {
		assert(renderPass);

        if (!isUploaded())
            return;

        SDL_GPUBufferBinding vertexBinding = {};
        vertexBinding.buffer = _vertexBuffer;
        vertexBinding.offset = 0;
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

        SDL_GPUBufferBinding indexBinding = {};
        indexBinding.buffer = _indexBuffer;
        SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    }

    void Mesh::draw(SDL_GPURenderPass* renderPass) const 
    {
        if (!isUploaded() || !renderPass)
            return;

        SDL_DrawGPUIndexedPrimitives(renderPass, static_cast<Uint32>(indexCount()), 1, 0, 0, 0);
    }

    const noz::bounds3& Mesh::bounds()
    {
        if (!_boundsCalculated)
            updateBounds();

        return _bounds;
    }

    void Mesh::updateBounds()
    {
        if (_positions.empty())
            _bounds = noz::bounds3();
        else
            _bounds = noz::bounds3::from_vertices(_positions);

        _boundsCalculated = true;
    }
}

