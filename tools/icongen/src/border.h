#pragma once

namespace icongen
{
    // Legacy RGBA struct for compatibility (can be removed later)
    struct RGBA 
    {
        uint8_t r, g, b, a;
        RGBA(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 0);
        
        // Conversion to/from noz::Color32
        operator noz::Color32() const { return noz::Color32(r, g, b, a); }
        RGBA(const noz::Color32& color) : r(color.r), g(color.g), b(color.b), a(color.a) {}
    };

    // Legacy function for compatibility
    std::vector<RGBA> addAntiAliasedBorder(const std::vector<RGBA>& pixels,
        int width, int height,
        float borderWidth,
        const RGBA& borderColor);

    // New Image-based border function
    noz::Image addAntiAliasedBorder(const noz::Image& image,
        float borderWidth,
        const noz::Color32& borderColor);
}