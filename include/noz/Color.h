/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz
{
    struct Color32;
    struct Color24;
    struct Color;

    struct Color32
    {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;

        // Constructors
        Color32() : r(0), g(0), b(0), a(255) {}
        Color32(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
        
        // Conversion constructors
        explicit Color32(const Color& color);
        explicit Color32(const Color24& color, uint8_t alpha = 255);

        // Utility methods
        bool isTransparent() const { return a == 0; }
        bool isOpaque() const { return a == 255; }
        
        // Common colors
        static const Color32 Black;
        static const Color32 White;
        static const Color32 Red;
        static const Color32 Green;
        static const Color32 Blue;
        static const Color32 Transparent;

        // Operators
        bool operator==(const Color32& other) const { return r == other.r && g == other.g && b == other.b && a == other.a; }
        bool operator!=(const Color32& other) const { return !(*this == other); }
    };

    /**
     * @brief 24-bit color without alpha (RGB format)
     * Each component is stored as a byte (0-255)
     */
    struct Color24
    {
        uint8_t r, g, b;

        // Constructors
        Color24() : r(0), g(0), b(0) {}
        Color24(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
        
        // Conversion constructors
        explicit Color24(const Color& color);
        explicit Color24(const Color32& color) : r(color.r), g(color.g), b(color.b) {}

        // Common colors
        static const Color24 Black;
        static const Color24 White;
        static const Color24 Red;
        static const Color24 Green;
        static const Color24 Blue;

        // Operators
        bool operator==(const Color24& other) const { return r == other.r && g == other.g && b == other.b; }
        bool operator!=(const Color24& other) const { return !(*this == other); }
    };

    /**
     * @brief Floating-point color (RGBA format)
     * Each component is stored as a float (0.0-1.0)
     * Used for most rendering operations
     */
    struct Color
    {
        float r, g, b, a;

        // Constructors
        Color() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}
        Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
        
        // Conversion constructors
        explicit Color(const Color32& color) 
            : r(color.r / 255.0f), g(color.g / 255.0f), b(color.b / 255.0f), a(color.a / 255.0f) {}
        explicit Color(const Color24& color, float alpha = 1.0f) 
            : r(color.r / 255.0f), g(color.g / 255.0f), b(color.b / 255.0f), a(alpha) {}

        // Utility methods
        bool isTransparent() const { return a <= 0.0f; }
        bool isOpaque() const { return a >= 1.0f; }
        
        // Clamp components to valid range
        Color clamped() const 
        {
            return Color(
                std::clamp(r, 0.0f, 1.0f),
                std::clamp(g, 0.0f, 1.0f),
                std::clamp(b, 0.0f, 1.0f),
                std::clamp(a, 0.0f, 1.0f)
            );
        }

        operator vec4() const { return vec4(r, g, b, a); }
        operator vec3() const { return vec3(r, g, b); }
		operator SDL_FColor() const { return SDL_FColor{ r, g, b, a }; }

        // Common colors
        static const Color Black;
        static const Color White;
        static const Color Red;
        static const Color Green;
        static const Color Blue;
        static const Color Transparent;

        // Operators
        bool operator==(const Color& other) const 
        { 
            const float epsilon = 1e-6f;
            return std::abs(r - other.r) < epsilon && 
                   std::abs(g - other.g) < epsilon && 
                   std::abs(b - other.b) < epsilon && 
                   std::abs(a - other.a) < epsilon; 
        }
        bool operator!=(const Color& other) const { return !(*this == other); }
        
        Color operator+(const Color& other) const { return Color(r + other.r, g + other.g, b + other.b, a + other.a); }
        Color operator-(const Color& other) const { return Color(r - other.r, g - other.g, b - other.b, a - other.a); }
        Color operator*(float scalar) const { return Color(r * scalar, g * scalar, b * scalar, a * scalar); }
        Color operator*(const Color& other) const { return Color(r * other.r, g * other.g, b * other.b, a * other.a); }
    };

    // Inline conversion implementations
    inline Color32::Color32(const Color& color) 
        : r(static_cast<uint8_t>(std::clamp(color.r * 255.0f, 0.0f, 255.0f)))
        , g(static_cast<uint8_t>(std::clamp(color.g * 255.0f, 0.0f, 255.0f)))
        , b(static_cast<uint8_t>(std::clamp(color.b * 255.0f, 0.0f, 255.0f)))
        , a(static_cast<uint8_t>(std::clamp(color.a * 255.0f, 0.0f, 255.0f))) {}

    inline Color32::Color32(const Color24& color, uint8_t alpha)
        : r(color.r), g(color.g), b(color.b), a(alpha) {}

    inline Color24::Color24(const Color& color)
        : r(static_cast<uint8_t>(std::clamp(color.r * 255.0f, 0.0f, 255.0f)))
        , g(static_cast<uint8_t>(std::clamp(color.g * 255.0f, 0.0f, 255.0f)))
        , b(static_cast<uint8_t>(std::clamp(color.b * 255.0f, 0.0f, 255.0f))) {}

} // namespace noz