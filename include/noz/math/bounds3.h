#pragma once

namespace noz
{
    /**
     * @brief 3D axis-aligned bounding box following GLM naming conventions
     */
    class bounds3
    {
    public:
        bounds3() = default;
        
        // Create bounds from center and size
        bounds3(const glm::vec3& center, float size);
        
        // Create bounds from center and extents
        bounds3(const glm::vec3& center, const glm::vec3& extents);
        
        // Create 2D bounds from position and size (z = 0)
        bounds3(float x, float y, float width, float height);
        
        // Calculate bounds from vertex positions
        static bounds3 from_vertices(const std::vector<glm::vec3>& positions);
        
        // Getters
        const glm::vec3& min() const { return _min; }
        const glm::vec3& max() const { return _max; }
		glm::vec3& min() { return _min; }
		glm::vec3& max() { return _max; }
		glm::vec3 center() const { return (_min + _max) * 0.5f; }
        glm::vec3 size() const { return _max - _min; }
        glm::vec3 extents() const { return (_max - _min) * 0.5f; }
        
        // Check if bounds are valid (min <= max for all components)
        bool valid() const;
        
        // Check if a point is inside the bounds
        bool contains(const glm::vec3& point) const;
        
        // Check if bounds intersect with another bounds
        bool intersects(const bounds3& other) const;
        
        // Expand bounds to include a point
        void expand(const glm::vec3& point);
        
        // Expand bounds to include another bounds
        void expand(const bounds3& other);
        
        // Transform bounds by a matrix
        bounds3 transform(const glm::mat4& matrix) const;
        
        // Get the volume of the bounds
        float volume() const;
        
        // Get the surface area of the bounds
        float surface_area() const;
        
    private:
        glm::vec3 _min = glm::vec3(std::numeric_limits<float>::infinity());
        glm::vec3 _max = glm::vec3(-std::numeric_limits<float>::infinity());
    };
    
} // namespace noz 