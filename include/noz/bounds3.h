//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once


#if 0

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
bounds3 transform(const mat4& matrix) const;

// Get the volume of the bounds
float volume() const;

// Get the surface area of the bounds
float surface_area() const;

#endif