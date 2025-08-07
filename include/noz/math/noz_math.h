#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/compatibility.hpp>

using namespace glm;

// Prevent Windows.h min/max macros from interfering with std::min/max
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif

namespace noz
{
    // ============================================================================
    // Math Constants
    // ============================================================================
    
    constexpr float PI = 3.14159265359f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float HALF_PI = PI * 0.5f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    
    inline const float EPSILON = 1e-6f;
    #ifdef INFINITY
    #undef INFINITY
    #endif
    inline const float INFINITY = std::numeric_limits<float>::infinity();
    inline const float NEG_INFINITY = -std::numeric_limits<float>::infinity();
    
    // ============================================================================
    // Common Vector Constants
    // ============================================================================
    
    inline const glm::vec2 VEC2_ZERO = glm::vec2(0.0f, 0.0f);
    inline const glm::vec2 VEC2_ONE = glm::vec2(1.0f, 1.0f);
    inline const glm::vec2 VEC2_UP = glm::vec2(0.0f, 1.0f);
    inline const glm::vec2 VEC2_DOWN = glm::vec2(0.0f, -1.0f);
    inline const glm::vec2 VEC2_LEFT = glm::vec2(-1.0f, 0.0f);
    inline const glm::vec2 VEC2_RIGHT = glm::vec2(1.0f, 0.0f);
    
    inline const glm::vec3 VEC3_ZERO = glm::vec3(0.0f, 0.0f, 0.0f);
    inline const glm::vec3 VEC3_ONE = glm::vec3(1.0f, 1.0f, 1.0f);
    inline const glm::vec3 VEC3_UP = glm::vec3(0.0f, 1.0f, 0.0f);
    inline const glm::vec3 VEC3_DOWN = glm::vec3(0.0f, -1.0f, 0.0f);
    inline const glm::vec3 VEC3_LEFT = glm::vec3(-1.0f, 0.0f, 0.0f);
    inline const glm::vec3 VEC3_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
    inline const glm::vec3 VEC3_FORWARD = glm::vec3(0.0f, 0.0f, -1.0f);
    inline const glm::vec3 VEC3_BACK = glm::vec3(0.0f, 0.0f, 1.0f);
    
    inline const glm::vec4 VEC4_ZERO = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    inline const glm::vec4 VEC4_ONE = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    
    // ============================================================================
    // Common Matrix Constants
    // ============================================================================
    
    inline const glm::mat2 MAT2_IDENTITY = glm::mat2(1.0f);
    inline const glm::mat3 MAT3_IDENTITY = glm::mat3(1.0f);
    inline const glm::mat4 MAT4_IDENTITY = glm::mat4(1.0f);
    
    // ============================================================================
    // Common Quaternion Constants
    // ============================================================================
    
    inline const glm::quat QUAT_IDENTITY = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    
} // namespace noz

namespace noz::math
{
    // ============================================================================
    // Math Utility Functions
    // ============================================================================
    
    // Angle conversions
    inline float radians(float degrees) { return degrees * noz::DEG_TO_RAD; }
    inline float degrees(float radians) { return radians * noz::RAD_TO_DEG; }
    
    // Clamping
    template<typename T>
    inline T clamp(T value, T min, T max) { return glm::clamp(value, min, max); }
    
    // Linear interpolation
    template<typename T>
    inline T lerp(T a, T b, float t) { return glm::mix(a, b, t); }
    
    // Mix (alias for lerp)
    template<typename T>
    inline T mix(T a, T b, float t) { return glm::mix(a, b, t); }
    
    // Smooth step interpolation
    inline float smoothstep(float edge0, float edge1, float x) { return glm::smoothstep(edge0, edge1, x); }
    
    // Distance
    inline float distance(const vec2& a, const vec2& b) { return glm::distance(a, b); }
    inline float distance(const vec3& a, const vec3& b) { return glm::distance(a, b); }
    inline float distance(const vec4& a, const vec4& b) { return glm::distance(a, b); }
    
    // Length
    inline float length(const vec2& v) { return glm::length(v); }
    inline float length(const vec3& v) { return glm::length(v); }
    inline float length(const vec4& v) { return glm::length(v); }
    
    // Normalize
    inline vec2 normalize(const vec2& v) { return glm::normalize(v); }
    inline vec3 normalize(const vec3& v) { return glm::normalize(v); }
    inline vec4 normalize(const vec4& v) { return glm::normalize(v); }
    
    // Dot product
    inline float dot(const vec2& a, const vec2& b) { return glm::dot(a, b); }
    inline float dot(const vec3& a, const vec3& b) { return glm::dot(a, b); }
    inline float dot(const vec4& a, const vec4& b) { return glm::dot(a, b); }
    
    // Cross product
    inline vec3 cross(const vec3& a, const vec3& b) { return glm::cross(a, b); }
        
    // Matrix transformations
    inline mat4 translate(const mat4& m, const vec3& v) { return glm::translate(m, v); }
    inline mat4 rotate(const mat4& m, float angle, const vec3& axis) { return glm::rotate(m, angle, axis); }
    inline mat4 scale(const mat4& m, const vec3& v) { return glm::scale(m, v); }
    inline mat4 perspective(float fov, float aspect, float near, float far) { return glm::perspective(fov, aspect, near, far); }
    inline mat4 ortho(float left, float right, float bottom, float top, float near, float far) { return glm::ortho(left, right, bottom, top, near, far); }
    inline mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) { return glm::lookAt(eye, center, up); }
    
    // Quaternion operations
    inline quat angleAxis(float angle, const vec3& axis) { return glm::angleAxis(angle, axis); }
    inline quat eulerAngles(float pitch, float yaw, float roll) { return glm::eulerAngleYXZ(yaw, pitch, roll); }
    inline vec3 eulerAngles(const quat& q) { return glm::eulerAngles(q); }
    
    // Utility functions
    inline bool isfinite(float x) { return std::isfinite(x); }
    inline bool isnan(float x) { return std::isnan(x); }
    inline float abs(float x) { return std::abs(x); }
    inline float sqrt(float x) { return std::sqrt(x); }
    inline float pow(float x, float y) { return std::pow(x, y); }
    inline float sin(float x) { return std::sin(x); }
    inline float cos(float x) { return std::cos(x); }
    inline float tan(float x) { return std::tan(x); }
    inline float asin(float x) { return std::asin(x); }
    inline float acos(float x) { return std::acos(x); }
    inline float atan(float x) { return std::atan(x); }
    inline float atan2(float y, float x) { return std::atan2(y, x); }

	inline double abs(double x) { return std::abs(x); }
	inline double sqrt(double x) { return std::sqrt(x); }
	inline double acos(double x) { return std::acos(x); }
	inline double cos(double x) { return std::cos(x); }
	inline double pow(double x, double y) { return std::pow(x, y); }

    // Min/Max
    template<typename T> inline T min(T a, T b) { return glm::min(a, b); }
    
    template<typename T> inline T max(T a, T b) { return glm::max(a, b); }
    
    // Approximate float comparison
    inline bool approximately(float a, float b, float epsilon = noz::EPSILON) 
    { 
        return std::abs(a - b) <= epsilon; 
    }
    
    // Vector component-wise min/max
    inline vec2 min(const vec2& a, const vec2& b) { return glm::min(a, b); }
    inline vec3 min(const vec3& a, const vec3& b) { return glm::min(a, b); }
    inline vec4 min(const vec4& a, const vec4& b) { return glm::min(a, b); }
    
    inline vec2 max(const vec2& a, const vec2& b) { return glm::max(a, b); }
    inline vec3 max(const vec3& a, const vec3& b) { return glm::max(a, b); }
    inline vec4 max(const vec4& a, const vec4& b) { return glm::max(a, b); }

	inline unsigned int nextPowerOf2(unsigned int n)
	{
		if (n == 0) return 1; // 2^0 is 1
		// If n is already a power of 2, return n
		if ((n & (n - 1)) == 0) return n;

		unsigned int p = 1;
		while (p < n) 
		{
			p <<= 1; // Left shift to multiply by 2
		}

		return p;
	}

} // namespace noz::math 

#include "bounds3.h"

