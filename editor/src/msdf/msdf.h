#pragma once

namespace noz::msdf
{
    struct Shape;

    void RenderShape(
        Shape* shape,
        std::vector<uint8_t>& output,
        int output_stride,
        const Vec2Int& output_pos,
        const Vec2Int& output_size,
        double range,
        const Vec2Double& scale,
        const Vec2Double& translate,
        int component_stride = 1,
        int component_offset = 0);
}
