//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <glm/vec2.hpp>

namespace noz
{
    constexpr float PI = 3.14159265359f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float HALF_PI = PI * 0.5f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;

    constexpr int MB = 1024 * 1024;

    // @power
    constexpr uint32_t NextPowerOf2(uint32_t n)
    {
        if (n <= 1) return 1;
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        return n + 1;
    }

    constexpr size_t NextPowerOf2(size_t n)
    {
        if (n <= 1) return 2;
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return n + 1;
    }

    // @rounding
    constexpr int RoundToNearest(float v)
    {
        return (int)(v + 0.5f);
    }

    constexpr glm::ivec2 RoundToNearest(const glm::vec2& v)
    {
        return glm::ivec2((int)(v.x + 0.5f), (int)(v.y + 0.5f));
    }

    constexpr glm::ivec2 RoundToNearest(const glm::dvec2& v)
    {
        return glm::ivec2((int)(v.x + 0.5f), (int)(v.y + 0.5f));
    }
}

float RandomFloat();
float RandomFloat(float min, float max);
int RandomInt(int min, int max);
bool RandomBool();
bool RandomBool(float probability);
