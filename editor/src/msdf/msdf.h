#pragma once

#include "../ttf/TrueTypeFont.h"

namespace noz::msdf
{
struct Shape;

void generateSDF(
    std::vector<uint8_t>& output,
    int outputStride,
    const Vec2Int& outputPosition,
    const Vec2Int& outputSize,
    const Shape& shape,
    double range,
    const Vec2Double& scale,
    const Vec2Double& translate);

void renderGlyph(
    const ttf::TrueTypeFont::Glyph* glyph,
    std::vector<uint8_t>& output,
    int outputStride,
    const Vec2Int& outputPosition,
    const Vec2Int& outputSize,
    double range,
    const Vec2Double& scale,
    const Vec2Double& translate);
}
