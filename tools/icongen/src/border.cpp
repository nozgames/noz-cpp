#include "icongen_pch.h"
#include "border.h"

namespace icongen
{
    RGBA::RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        : r(r), g(g), b(b), a(a) {}

    std::vector<RGBA> addAntiAliasedBorder(const std::vector<RGBA>& pixels,
        int width, int height,
        float borderWidth,
        const RGBA& borderColor) 
    {
        std::vector<RGBA> result = pixels;  // Copy original

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = y * width + x;

                // Skip pixels that are already part of the model
                if (pixels[idx].a > 0) {
                    continue;
                }

                // Find distance to nearest opaque pixel
                float minDistance = borderWidth + 1.0f;

                int searchRadius = static_cast<int>(std::ceil(borderWidth));
                for (int dy = -searchRadius; dy <= searchRadius; dy++) {
                    for (int dx = -searchRadius; dx <= searchRadius; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;

                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int nIdx = ny * width + nx;
                            if (pixels[nIdx].a > 0) {  // Found opaque pixel
                                float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                                minDistance = std::min(minDistance, distance);
                            }
                        }
                    }
                }

                // If within border range, add anti-aliased border
                if (minDistance <= borderWidth) {
                    float alpha = 1.0f - (minDistance / borderWidth);
                    alpha = std::clamp(alpha, 0.0f, 1.0f);

                    result[idx].r = borderColor.r;
                    result[idx].g = borderColor.g;
                    result[idx].b = borderColor.b;
                    result[idx].a = static_cast<uint8_t>(borderColor.a * alpha);
                }
            }
        }

        return result;
    }

    noz::Image addAntiAliasedBorder(const noz::Image& image, float borderWidth, const noz::Color32& borderColor)
    {
        if (image.empty() || borderWidth <= 0.0f)
            return image; // Return copy of original image

        int width = image.width();
        int height = image.height();
        
        // Create result image (copy of original)
        noz::Image result = image;

        // Apply border effect
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                noz::Color32 currentPixel = image.getPixel32(x, y);
                
                // Skip pixels that are already opaque (part of the model)
                if (currentPixel.a > 0)
                    continue;

                // Find distance to nearest opaque pixel
                float minDistance = borderWidth + 1.0f;
                int searchRadius = static_cast<int>(std::ceil(borderWidth));
                
                for (int dy = -searchRadius; dy <= searchRadius; dy++)
                {
                    for (int dx = -searchRadius; dx <= searchRadius; dx++)
                    {
                        int nx = x + dx;
                        int ny = y + dy;

                        if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                        {
                            noz::Color32 neighborPixel = image.getPixel32(nx, ny);
                            if (neighborPixel.a > 0) // Found opaque pixel
                            {
                                float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                                minDistance = std::min(minDistance, distance);
                            }
                        }
                    }
                }

                // If within border range, add anti-aliased border
                if (minDistance <= borderWidth)
                {
                    float alpha = 1.0f - (minDistance / borderWidth);
                    alpha = std::clamp(alpha, 0.0f, 1.0f);

                    noz::Color32 borderedPixel = borderColor;
                    borderedPixel.a = static_cast<uint8_t>(borderColor.a * alpha);
                    result.setPixel(x, y, borderedPixel);
                }
            }
        }

        return result;
    }
}