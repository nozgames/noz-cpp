/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/renderer/Mesh.h>

namespace noz::renderer
{
    MeshBuilder::MeshBuilder(int initialSize)
    {
        _positions.reserve(initialSize);
        _normals.reserve(initialSize);
        _uv0.reserve(initialSize);
        _boneIndices.reserve(initialSize);
        _indices.reserve(initialSize * 3);
	}

    void MeshBuilder::clear()
    {
        _positions.clear();
        _normals.clear();
        _uv0.clear();
        _boneIndices.clear();
        _indices.clear();
    }

    void MeshBuilder::addVertex(
		const vec3& position,
		const vec3& normal,
		const vec2& uv,
		uint32_t boneIndex)
    {
        _positions.push_back(position);
        _normals.push_back(normal);
        _uv0.push_back(uv);
        _boneIndices.push_back(boneIndex);
    }

    void MeshBuilder::addIndex(uint16_t index)
    {
        _indices.push_back(index);
    }

    void MeshBuilder::addTriangle(uint16_t a, uint16_t b, uint16_t c)
    {
        _indices.push_back(a);
        _indices.push_back(b);
        _indices.push_back(c);
    }

    void MeshBuilder::addTriangle(
		const glm::vec3& a,
		const glm::vec3& b,
		const glm::vec3& c,
		uint32_t boneIndex)
    {
        // Calculate face normal
        glm::vec3 v1 = b - a;
        glm::vec3 v2 = c - a;
        glm::vec3 normal = glm::normalize(glm::cross(v2, v1));
        
        // Add vertices with computed normal
        uint16_t indexA = static_cast<uint16_t>(_positions.size());
        addVertex(a, normal, glm::vec2(0.0f, 0.0f), boneIndex);
        
        uint16_t indexB = static_cast<uint16_t>(_positions.size());
        addVertex(b, normal, glm::vec2(1.0f, 0.0f), boneIndex);
        
        uint16_t indexC = static_cast<uint16_t>(_positions.size());
        addVertex(c, normal, glm::vec2(0.5f, 1.0f), boneIndex);
        
        // Add triangle indices
        addTriangle(indexA, indexB, indexC);
    }

    void MeshBuilder::addPyramid(const glm::vec3& start, const glm::vec3& end, float size, uint32_t boneIndex)
    {
        // Calculate direction and create base
        glm::vec3 direction = glm::normalize(end - start);
        float length = glm::distance(start, end);
        
        // Create rotation matrix to align with direction
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        if (glm::abs(glm::dot(direction, up)) > 0.9f)
            up = glm::vec3(1.0f, 0.0f, 0.0f);
        
        glm::vec3 right = glm::normalize(glm::cross(direction, up));
        up = glm::normalize(glm::cross(right, direction));
        
		auto hsize = size * 0.5f;
		addTriangle(
			start + right * hsize + up * hsize,
			start + right * hsize - up * hsize,
			end,
			boneIndex);

		addTriangle(
			start - right * hsize + up * hsize,
			start + right * hsize + up * hsize,
			end,
			boneIndex);

		addTriangle(
			start - right * hsize - up * hsize,
			start - right * hsize + up * hsize,
			end,
			boneIndex);

		addTriangle(
			start + right * hsize - up * hsize,
			start - right * hsize - up * hsize,
			end,
			boneIndex);
    }

    void MeshBuilder::addCube(const glm::vec3& center, const glm::vec3& size, uint32_t boneIndex)
    {
        glm::vec3 halfSize = size * 0.5f;
        
        // Define cube vertices
        std::vector<glm::vec3> vertices = {
            center + glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z), // 0: bottom-left-back
            center + glm::vec3( halfSize.x, -halfSize.y, -halfSize.z), // 1: bottom-right-back
            center + glm::vec3( halfSize.x,  halfSize.y, -halfSize.z), // 2: top-right-back
            center + glm::vec3(-halfSize.x,  halfSize.y, -halfSize.z), // 3: top-left-back
            center + glm::vec3(-halfSize.x, -halfSize.y,  halfSize.z), // 4: bottom-left-front
            center + glm::vec3( halfSize.x, -halfSize.y,  halfSize.z), // 5: bottom-right-front
            center + glm::vec3( halfSize.x,  halfSize.y,  halfSize.z), // 6: top-right-front
            center + glm::vec3(-halfSize.x,  halfSize.y,  halfSize.z)  // 7: top-left-front
        };
        
        // Add vertices
        uint32_t baseIndex = static_cast<uint32_t>(_positions.size());
        for (const auto& vertex : vertices)
        {
            addVertex(vertex, glm::vec3(0.0f), glm::vec2(0.0f), boneIndex);
        }
        
        // Define faces (6 faces, each with 2 triangles)
        std::vector<std::vector<uint32_t>> faces = {
            {0, 1, 2, 3}, // Back face
            {5, 4, 7, 6}, // Front face
            {4, 0, 3, 7}, // Left face
            {1, 5, 6, 2}, // Right face
            {3, 2, 6, 7}, // Top face
            {4, 5, 1, 0}  // Bottom face
        };
        
        // Face normals
        std::vector<glm::vec3> faceNormals = {
            glm::vec3( 0.0f,  0.0f, -1.0f), // Back
            glm::vec3( 0.0f,  0.0f,  1.0f), // Front
            glm::vec3(-1.0f,  0.0f,  0.0f), // Left
            glm::vec3( 1.0f,  0.0f,  0.0f), // Right
            glm::vec3( 0.0f,  1.0f,  0.0f), // Top
            glm::vec3( 0.0f, -1.0f,  0.0f)  // Bottom
        };
        
        // Create faces
        for (size_t i = 0; i < faces.size(); ++i)
        {
            const auto& face = faces[i];
            const auto& normal = faceNormals[i];
            
            // Update normals for this face
            for (uint32_t vertexIndex : face)
            {
                _normals[baseIndex + vertexIndex] = normal;
            }
            
            // Add triangles (each face is a quad, so 2 triangles)
            addTriangle(baseIndex + face[0], baseIndex + face[1], baseIndex + face[2]);
            addTriangle(baseIndex + face[0], baseIndex + face[2], baseIndex + face[3]);
        }
    }

    void MeshBuilder::addSphere(const glm::vec3& center, float radius, int segments, int rings, uint32_t boneIndex)
    {
        uint32_t baseIndex = static_cast<uint32_t>(_positions.size());
        
        // Generate sphere vertices
        for (int ring = 0; ring <= rings; ++ring)
        {
            float phi = (glm::pi<float>() * ring) / rings;
            float y = cos(phi);
            float ringRadius = sin(phi);
            
            for (int segment = 0; segment <= segments; ++segment)
            {
                float theta = (2.0f * glm::pi<float>() * segment) / segments;
                float x = cos(theta) * ringRadius;
                float z = sin(theta) * ringRadius;
                
                glm::vec3 position = center + glm::vec3(x, y, z) * radius;
                glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
                glm::vec2 uv = glm::vec2(static_cast<float>(segment) / segments, static_cast<float>(ring) / rings);
                
                addVertex(position, normal, uv, boneIndex);
            }
        }
        
        // Generate sphere indices
        for (int ring = 0; ring < rings; ++ring)
        {
            for (int segment = 0; segment < segments; ++segment)
            {
                int current = baseIndex + ring * (segments + 1) + segment;
                int next = current + 1;
                int below = current + (segments + 1);
                int belowNext = below + 1;
                
                // Two triangles per quad
                addTriangle(current, below, next);
                addTriangle(next, below, belowNext);
            }
        }
    }

    void MeshBuilder::addLine(const glm::vec3& start, const glm::vec3& end, float thickness, uint32_t boneIndex)
    {
        // Create a simple line as a thin cylinder
        glm::vec3 direction = glm::normalize(end - start);
        float length = glm::distance(start, end);
        
        // Create rotation matrix
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        if (glm::abs(glm::dot(direction, up)) > 0.9f)
            up = glm::vec3(1.0f, 0.0f, 0.0f);
        
        glm::vec3 right = glm::normalize(glm::cross(direction, up));
        up = glm::normalize(glm::cross(right, direction));
        
        // Create thin cylinder with 4 segments
        const int segments = 4;
        std::vector<uint32_t> startIndices, endIndices;
        
        // Add start vertices
        for (int i = 0; i < segments; ++i)
        {
            float angle = (2.0f * glm::pi<float>() * i) / segments;
            glm::vec3 offset = right * cos(angle) + up * sin(angle);
            glm::vec3 position = start + offset * thickness;
            glm::vec3 normal = glm::normalize(offset);
            
            startIndices.push_back(static_cast<uint32_t>(_positions.size()));
            addVertex(position, normal, glm::vec2(0.0f, static_cast<float>(i) / segments), boneIndex);
        }
        
        // Add end vertices
        for (int i = 0; i < segments; ++i)
        {
            float angle = (2.0f * glm::pi<float>() * i) / segments;
            glm::vec3 offset = right * cos(angle) + up * sin(angle);
            glm::vec3 position = end + offset * thickness;
            glm::vec3 normal = glm::normalize(offset);
            
            endIndices.push_back(static_cast<uint32_t>(_positions.size()));
            addVertex(position, normal, glm::vec2(1.0f, static_cast<float>(i) / segments), boneIndex);
        }
        
        // Create side faces
        for (int i = 0; i < segments; ++i)
        {
            int next = (i + 1) % segments;
            
            // Add quad for side face
            addQuad(
                glm::vec3(_positions[startIndices[i]]),
                glm::vec3(_positions[startIndices[next]]),
                glm::vec3(_positions[endIndices[next]]),
                glm::vec3(_positions[endIndices[i]]),
				glm::vec2(0,0),
                glm::normalize(glm::cross(
                    _positions[startIndices[next]] - _positions[startIndices[i]],
                    _positions[endIndices[i]] - _positions[startIndices[i]]
                )),
                boneIndex
            );
        }
    }

    void MeshBuilder::addQuad(
		const glm::vec3& a,
		const glm::vec3& b,
		const glm::vec3& c,
		const glm::vec3& d, 
		const glm::vec2& color,
        const glm::vec3& normal,
		uint32_t boneIndex)
    {
        uint32_t baseIndex = static_cast<uint32_t>(_positions.size());
        
        // Add vertices
        addVertex(a, normal, color, boneIndex);
        addVertex(b, normal, color, boneIndex);
        addVertex(c, normal, color, boneIndex);
        addVertex(d, normal, color, boneIndex);
        
        // Add triangles
        addTriangle(baseIndex, baseIndex + 1, baseIndex + 2);
        addTriangle(baseIndex, baseIndex + 2, baseIndex + 3);
    }

    void MeshBuilder::addTriangle(
		const glm::vec3& a,
		const glm::vec3& b,
		const glm::vec3& c, 
		const glm::vec2& color,
        const glm::vec3& normal,
		uint32_t boneIndex)
    {
        uint32_t baseIndex = static_cast<uint32_t>(_positions.size());
        
        // Add vertices
        addVertex(a, normal, color, boneIndex);
        addVertex(b, normal, color, boneIndex);
        addVertex(c, normal, color, boneIndex);
        
        // Add triangle
        addTriangle(baseIndex, baseIndex + 1, baseIndex + 2);
    }

    void MeshBuilder::addCylinder(const glm::vec3& start, const glm::vec3& end, float radius, const glm::vec2& colorUV, int segments, uint32_t boneIndex)
    {
        glm::vec3 direction = glm::normalize(end - start);
        float length = glm::distance(start, end);
        
        // Create rotation matrix to align with direction
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        if (glm::abs(glm::dot(direction, up)) > 0.9f)
            up = glm::vec3(1.0f, 0.0f, 0.0f);
        
        glm::vec3 right = glm::normalize(glm::cross(direction, up));
        up = glm::normalize(glm::cross(right, direction));
        
        uint32_t baseIndex = static_cast<uint32_t>(_positions.size());
        
        // Generate vertices for start and end circles
        std::vector<uint32_t> startIndices, endIndices;
        
        // Add center vertices for caps
        uint32_t startCenterIndex = static_cast<uint32_t>(_positions.size());
        addVertex(start, -direction, colorUV, boneIndex);
        
        uint32_t endCenterIndex = static_cast<uint32_t>(_positions.size());
        addVertex(end, direction, colorUV, boneIndex);
        
        // Generate circle vertices
        for (int i = 0; i < segments; ++i)
        {
            float angle = (2.0f * glm::pi<float>() * i) / segments;
            glm::vec3 offset = right * cos(angle) + up * sin(angle);
            glm::vec3 normal = glm::normalize(offset);
            
            // Start circle vertex
            glm::vec3 startPos = start + offset * radius;
            startIndices.push_back(static_cast<uint32_t>(_positions.size()));
            addVertex(startPos, -direction, colorUV, boneIndex);
            
            // End circle vertex
            glm::vec3 endPos = end + offset * radius;
            endIndices.push_back(static_cast<uint32_t>(_positions.size()));
            addVertex(endPos, direction, colorUV, boneIndex);
        }
        
        // Create start cap triangles
        for (int i = 0; i < segments; ++i)
        {
            int next = (i + 1) % segments;
            addTriangle(startIndices[i], startIndices[next], startCenterIndex);
        }
        
        // Create end cap triangles
        for (int i = 0; i < segments; ++i)
        {
            int next = (i + 1) % segments;
            //addTriangle(endCenterIndex, endIndices[i], endIndices[next]);
        }
        
        // Create side faces
        for (int i = 0; i < segments; ++i)
        {
            int next = (i + 1) % segments;
            
            // Calculate normal for this face
            glm::vec3 v1 = glm::vec3(_positions[startIndices[next]]) - glm::vec3(_positions[startIndices[i]]);
            glm::vec3 v2 = glm::vec3(_positions[endIndices[i]]) - glm::vec3(_positions[startIndices[i]]);
            glm::vec3 faceNormal = -glm::normalize(glm::cross(v1, v2));
            
            // Add quad for side face
            addQuad(
				glm::vec3(_positions[endIndices[i]]),
				glm::vec3(_positions[endIndices[next]]),
				glm::vec3(_positions[startIndices[next]]),
				glm::vec3(_positions[startIndices[i]]),
				colorUV,
                faceNormal,
                boneIndex
            );
        }
    }

    void MeshBuilder::addCone(
		const glm::vec3& base,
		const glm::vec3& tip,
		float baseRadius,
		const glm::vec2& colorUV,
		int segments,
		uint32_t boneIndex)
    {
        glm::vec3 direction = glm::normalize(tip - base);
        float length = glm::distance(base, tip);
        
        // Create rotation matrix to align with direction
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        if (glm::abs(glm::dot(direction, up)) > 0.9f)
            up = glm::vec3(1.0f, 0.0f, 0.0f);
        
        glm::vec3 right = glm::normalize(glm::cross(direction, up));
        up = glm::normalize(glm::cross(right, direction));
        
        uint32_t baseIndex = static_cast<uint32_t>(_positions.size());
        
        // Add base center vertex
        uint32_t baseCenterIndex = static_cast<uint32_t>(_positions.size());
        addVertex(base, -direction, colorUV, boneIndex);
        
        // Add tip vertex
        uint32_t tipIndex = static_cast<uint32_t>(_positions.size());
        addVertex(tip, direction, colorUV, boneIndex);
        
        // Generate base circle vertices
        std::vector<uint32_t> baseIndices;
        for (int i = 0; i < segments; ++i)
        {
            float angle = (2.0f * glm::pi<float>() * i) / segments;
            glm::vec3 offset = right * cos(angle) + up * sin(angle);
            glm::vec3 basePos = base + offset * baseRadius;
            
            baseIndices.push_back(static_cast<uint32_t>(_positions.size()));
            addVertex(basePos, -direction, colorUV, boneIndex);
        }
        
        // Create base cap triangles
        for (int i = 0; i < segments; ++i)
        {
            int next = (i + 1) % segments;
            addTriangle(baseIndices[i], baseIndices[next], baseCenterIndex);
        }
        
        // Create side triangles (from base to tip)
        for (int i = 0; i < segments; ++i)
        {
            int next = (i + 1) % segments;
            
            // Calculate normal for this face
            auto v1 = glm::vec3(_positions[baseIndices[next]]) - glm::vec3(_positions[baseIndices[i]]);
			auto v2 = glm::vec3(_positions[tipIndex]) - glm::vec3(_positions[baseIndices[i]]);
			auto n = -glm::normalize(glm::cross(v1, v2));
            
            // Add triangle
            addTriangle(
				glm::vec3(_positions[tipIndex]),
				glm::vec3(_positions[baseIndices[next]]),
				glm::vec3(_positions[baseIndices[i]]),
                colorUV,
				n,
                boneIndex
            );
        }
    }

	std::shared_ptr<class Mesh> MeshBuilder::toMesh(const std::string& name)
	{
		if (!hasData())
			return nullptr;

		auto mesh = std::make_shared<Mesh>(name);
		mesh->positions() = _positions;
		mesh->normals() = _normals;
		mesh->uv0() = _uv0;
		mesh->boneIndices() = _boneIndices;
		mesh->indices() = _indices;
		return mesh;
	}

    std::shared_ptr<Mesh> MeshBuilder::build(const std::string& name)
    {
        if (!hasData())
        {
            return nullptr;
        }

        // Create a new mesh
        auto mesh = std::make_shared<Mesh>(name);
        
        // Copy data from builder to mesh
        mesh->positions() = _positions;
        mesh->normals() = _normals;
        mesh->uv0() = _uv0;
        mesh->boneIndices() = _boneIndices;
        mesh->indices() = _indices;
        
        // Upload mesh data to GPU
        mesh->upload();
        
        return mesh;
    }

    void MeshBuilder::addMesh(const std::shared_ptr<Mesh>& mesh, const vec3& offset)
    {
        assert(mesh);

		_positions.reserve(_positions.size() + mesh->positions().size());
		for (const auto& pos : mesh->positions())
			_positions.push_back(pos + offset);

        _normals.insert(_normals.end(), mesh->normals().begin(), mesh->normals().end());
        _uv0.insert(_uv0.end(), mesh->uv0().begin(), mesh->uv0().end());
        _boneIndices.insert(_boneIndices.end(), mesh->boneIndices().begin(), mesh->boneIndices().end());

		// Adjust indices to account for existing vertices
		uint32_t baseIndex = static_cast<uint32_t>(_positions.size() - mesh->positions().size());
		for (const auto& index : mesh->indices())
			_indices.push_back(static_cast<uint16_t>(index + baseIndex));
	}
}