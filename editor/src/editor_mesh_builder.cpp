//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    constexpr int EDITOR_CIRCLE_SEGMENTS = 16;

    void AddEditorLine(MeshBuilder* builder, const Vec2& v0, const Vec2& v1, float width, const Color& color) {
        SetBaseVertex(builder);
        Vec2 dir = v1 - v0;
        float length = Length(dir);
        if (length < F32_EPSILON) return;
        dir = dir / length;
        Vec2 perp = {-dir.y, dir.x};
        float half_width = width * 0.5f;
        Vec2 p0 = v0 - perp * half_width;
        Vec2 p1 = v1 - perp * half_width;
        Vec2 p2 = v1 + perp * half_width;
        Vec2 p3 = v0 + perp * half_width;
        Vec4 color_vec = {color.r, color.g, color.b, color.a};
        MeshVertex mv0 = {.position = p0, .color = color_vec};
        MeshVertex mv1 = {.position = p1, .color = color_vec};
        MeshVertex mv2 = {.position = p2, .color = color_vec};
        MeshVertex mv3 = {.position = p3, .color = color_vec};
        AddVertex(builder, mv0);
        AddVertex(builder, mv1);
        AddVertex(builder, mv2);
        AddVertex(builder, mv3);
        AddTriangle(builder, 0, 1, 2);
        AddTriangle(builder, 0, 2, 3);
    }

    void AddEditorSquare(MeshBuilder* builder, const Vec2& center, float size, const Color& color) {
        SetBaseVertex(builder);
        float half = size * 0.5f;
        Vec4 color_vec = {color.r, color.g, color.b, color.a};
        MeshVertex v0 = {.position = {center.x - half, center.y - half}, .color = color_vec};
        MeshVertex v1 = {.position = {center.x + half, center.y - half}, .color = color_vec};
        MeshVertex v2 = {.position = {center.x + half, center.y + half}, .color = color_vec};
        MeshVertex v3 = {.position = {center.x - half, center.y + half}, .color = color_vec};
        AddVertex(builder, v0);
        AddVertex(builder, v1);
        AddVertex(builder, v2);
        AddVertex(builder, v3);
        AddTriangle(builder, 0, 1, 2);
        AddTriangle(builder, 0, 2, 3);
    }

    void AddEditorCircle(MeshBuilder* builder, const Vec2& center, float radius, const Color& color) {
        SetBaseVertex(builder);
        Vec4 color_vec = {color.r, color.g, color.b, color.a};
        MeshVertex center_vert = {.position = center, .color = color_vec};
        AddVertex(builder, center_vert);
        for (int i = 0; i <= EDITOR_CIRCLE_SEGMENTS; ++i) {
            float angle = static_cast<float>(i) / static_cast<float>(EDITOR_CIRCLE_SEGMENTS) * noz::PI * 2.0f;
            Vec2 offset = {cosf(angle) * radius, sinf(angle) * radius};
            MeshVertex v = {.position = center + offset, .color = color_vec};
            AddVertex(builder, v);
        }
        for (int i = 0; i < EDITOR_CIRCLE_SEGMENTS; ++i)
            AddTriangle(builder, 0, (u16)(i + 1), (u16)(i + 2));
    }

    void AddEditorCircleStroke(MeshBuilder* builder, const Vec2& center, float radius, float thickness, const Color& color) {
        SetBaseVertex(builder);
        Vec4 color_vec = {color.r, color.g, color.b, color.a};
        float inner_radius = radius - thickness * 0.5f;
        float outer_radius = radius + thickness * 0.5f;
        float step = 2.0f * noz::PI / (float)EDITOR_CIRCLE_SEGMENTS;
        for (int i = 0; i <= EDITOR_CIRCLE_SEGMENTS; ++i) {
            float angle = i * step;
            Vec2 offset_inner = {cosf(angle) * inner_radius, sinf(angle) * inner_radius};
            Vec2 offset_outer = {cosf(angle) * outer_radius, sinf(angle) * outer_radius};
            MeshVertex vi = {.position = center + offset_inner, .color = color_vec};
            MeshVertex vo = {.position = center + offset_outer, .color = color_vec};
            AddVertex(builder, vi);
            AddVertex(builder, vo);
        }
        for (int i = 0; i < EDITOR_CIRCLE_SEGMENTS; ++i) {
            u16 i0 = (u16)(i * 2 + 0);
            u16 i1 = (u16)(i * 2 + 1);
            u16 i2 = (u16)(i * 2 + 2);
            u16 i3 = (u16)(i * 2 + 3);
            AddTriangle(builder, i0, i1, i2);
            AddTriangle(builder, i2, i1, i3);
        }
    }

    void AddEditorArc(MeshBuilder* builder, const Vec2& center, float radius, float fill_percent, const Color& color) {
        if (fill_percent <= 0.0f) return;
        fill_percent = Clamp01(fill_percent);
        SetBaseVertex(builder);
        Vec4 color_vec = {color.r, color.g, color.b, color.a};
        MeshVertex center_vert = {.position = center, .color = color_vec};
        AddVertex(builder, center_vert);
        int arc_segments = Max(1, (int)(EDITOR_CIRCLE_SEGMENTS * fill_percent));
        float angle_end = fill_percent * noz::PI * 2.0f;
        for (int i = 0; i <= arc_segments; ++i) {
            float angle = -noz::PI * 0.5f + (float)i / (float)arc_segments * angle_end;
            Vec2 offset = {cosf(angle) * radius, sinf(angle) * radius};
            MeshVertex v = {.position = center + offset, .color = color_vec};
            AddVertex(builder, v);
        }
        for (int i = 0; i < arc_segments; ++i)
            AddTriangle(builder, 0, (u16)(i + 1), (u16)(i + 2));
    }

    void AddEditorBone(MeshBuilder* builder, const Vec2& a, const Vec2& b, float line_width, const Color& color) {
        float l = Length(b - a);
        if (l < F32_EPSILON) return;
        float s = l * BONE_WIDTH;
        Vec2 d = Normalize(b - a);
        Vec2 c = a + d * s;
        Vec2 n = Perpendicular(d);
        Vec2 aa = c + n * s;
        Vec2 bb = c - n * s;
        AddEditorLine(builder, a, bb, line_width, color);
        AddEditorLine(builder, a, aa, line_width, color);
        AddEditorLine(builder, aa, b, line_width, color);
        AddEditorLine(builder, bb, b, line_width, color);
    }

    void AddEditorDashedLine(MeshBuilder* builder, const Vec2& v0, const Vec2& v1, float width, float dash_length, const Color& color) {
        Vec2 dir = v1 - v0;
        float total_length = Length(dir);
        if (total_length < F32_EPSILON) return;
        dir = dir / total_length;
        float gap_length = dash_length;
        float pos = 0.0f;
        bool draw = true;
        while (pos < total_length) {
            float segment_length = draw ? dash_length : gap_length;
            float end_pos = Min(pos + segment_length, total_length);
            if (draw) {
                Vec2 start = v0 + dir * pos;
                Vec2 end = v0 + dir * end_pos;
                AddEditorLine(builder, start, end, width, color);
            }
            pos = end_pos;
            draw = !draw;
        }
    }
}
