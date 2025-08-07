#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace noz::renderer
{
    /**
     * @brief Helper class for building meshes with various geometric primitives
     * Provides methods to add vertices, indices, and common shapes
     */
    class MeshBuilder
    {
    public:
        MeshBuilder() = default;
        ~MeshBuilder() = default;

        // Clear all data
        void clear();

        // Basic vertex/index operations
        void addVertex(const glm::vec3& position, const glm::vec3& normal = glm::vec3(0.0f), const glm::vec2& uv = glm::vec2(0.0f), uint32_t boneIndex = 0);
        void addIndex(uint16_t index);
        void addTriangle(uint16_t a, uint16_t b, uint16_t c);
        void addTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, uint32_t boneIndex = 0);

        // Geometric primitives
        void addPyramid(const glm::vec3& start, const glm::vec3& end, float size, uint32_t boneIndex = 0);
        void addCube(const glm::vec3& center, const glm::vec3& size, uint32_t boneIndex = 0);
        void addSphere(const glm::vec3& center, float radius, int segments = 8, int rings = 4, uint32_t boneIndex = 0);
        void addLine(const glm::vec3& start, const glm::vec3& end, float thickness = 0.01f, uint32_t boneIndex = 0);
        void addCylinder(const glm::vec3& start, const glm::vec3& end, float radius, const glm::vec2& colorUV, int segments = 8, uint32_t boneIndex = 0);
        void addCone(const glm::vec3& base, const glm::vec3& tip, float baseRadius, const glm::vec2& colorUV, int segments = 8, uint32_t boneIndex = 0);

        // Get the built mesh data
        const std::vector<glm::vec3>& positions() const { return _positions; }
        const std::vector<glm::vec3>& normals() const { return _normals; }
        const std::vector<glm::vec2>& uv0() const { return _uv0; }
        const std::vector<uint32_t>& boneIndices() const { return _boneIndices; }
        const std::vector<uint16_t>& indices() const { return _indices; }

        // Get vertex count
        size_t vertexCount() const { return _positions.size(); }
        size_t indexCount() const { return _indices.size(); }

        // Check if mesh has data
        bool hasData() const { return !_positions.empty(); }

        // Build a mesh from the current data
        std::shared_ptr<class Mesh> build(const std::string& name = "GeneratedMesh");

		std::shared_ptr<class Mesh> toMesh(const std::string& name = "MeshBuilderMesh");

    private:
        
        void addQuad(
			const glm::vec3& a,
			const glm::vec3& b,
			const glm::vec3& c,
			const glm::vec3& d, 
			const glm::vec2& color,
			const glm::vec3& normal,
			uint32_t boneIndex = 0);

        void addTriangle(
			const glm::vec3& a,
			const glm::vec3& b,
			const glm::vec3& c, 
			const glm::vec2& color,
            const glm::vec3& normal,
			uint32_t boneIndex = 0);

        // Mesh data
        std::vector<glm::vec3> _positions;
        std::vector<glm::vec3> _normals;
        std::vector<glm::vec2> _uv0;
        std::vector<uint32_t> _boneIndices;
        std::vector<uint16_t> _indices;
    };

} // namespace noz::renderer 