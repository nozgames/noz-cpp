/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
    bounds3::bounds3(const glm::vec3& center, float size)
    {
        float half_size = size * 0.5f;
        _min = center - glm::vec3(half_size);
        _max = center + glm::vec3(half_size);
    }
    
    bounds3::bounds3(const glm::vec3& center, const glm::vec3& extents)
    {
        _min = center - extents;
        _max = center + extents;
    }
    
    bounds3::bounds3(float x, float y, float width, float height)
    {
        _min = glm::vec3(x, y, 0.0f);
        _max = glm::vec3(x + width, y + height, 0.0f);
    }
    
    bounds3 bounds3::from_vertices(const std::vector<glm::vec3>& positions)
    {
        if (positions.empty())
            return bounds3();
            
        glm::vec3 min_pos = positions[0];
        glm::vec3 max_pos = positions[0];
        
        for (const auto& pos : positions)
        {
            min_pos = glm::min(min_pos, pos);
            max_pos = glm::max(max_pos, pos);
        }
        
        glm::vec3 center = (min_pos + max_pos) * 0.5f;
        glm::vec3 extents = (max_pos - min_pos) * 0.5f;
        return bounds3(center, extents);
    }
    
    bool bounds3::valid() const
    {
        return _min.x <= _max.x && _min.y <= _max.y && _min.z <= _max.z;
    }
    
    bool bounds3::contains(const glm::vec3& point) const
    {
        return point.x >= _min.x && point.x <= _max.x &&
               point.y >= _min.y && point.y <= _max.y &&
               point.z >= _min.z && point.z <= _max.z;
    }
    
    bool bounds3::intersects(const bounds3& other) const
    {
        return _min.x <= other._max.x && _max.x >= other._min.x &&
               _min.y <= other._max.y && _max.y >= other._min.y &&
               _min.z <= other._max.z && _max.z >= other._min.z;
    }
    
    void bounds3::expand(const glm::vec3& point)
    {
        _min = glm::min(_min, point);
        _max = glm::max(_max, point);
    }
    
    void bounds3::expand(const bounds3& other)
    {
        _min = glm::min(_min, other._min);
        _max = glm::max(_max, other._max);
    }
    
    bounds3 bounds3::transform(const glm::mat4& matrix) const
    {
        // Transform all 8 corners of the bounding box
        std::vector<glm::vec3> corners = {
            glm::vec3(_min.x, _min.y, _min.z),
            glm::vec3(_max.x, _min.y, _min.z),
            glm::vec3(_min.x, _max.y, _min.z),
            glm::vec3(_max.x, _max.y, _min.z),
            glm::vec3(_min.x, _min.y, _max.z),
            glm::vec3(_max.x, _min.y, _max.z),
            glm::vec3(_min.x, _max.y, _max.z),
            glm::vec3(_max.x, _max.y, _max.z)
        };
        
        // Transform all corners
        for (auto& corner : corners)
        {
            glm::vec4 transformed = matrix * glm::vec4(corner, 1.0f);
            corner = glm::vec3(transformed) / transformed.w;
        }
        
        // Calculate new bounds from transformed corners
        return from_vertices(corners);
    }
    
    float bounds3::volume() const
    {
        glm::vec3 size_vec = size();
        return size_vec.x * size_vec.y * size_vec.z;
    }
    
    float bounds3::surface_area() const
    {
        glm::vec3 size_vec = size();
        return 2.0f * (size_vec.x * size_vec.y + size_vec.y * size_vec.z + size_vec.z * size_vec.x);
    }
    
} // namespace noz 