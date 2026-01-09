//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "pixel_data.h"
#include "rasterizer.h"

constexpr int RASTER_MAX_VERTICES = 256;

struct RasterizerImpl : Rasterizer {
    PixelData* pixels = nullptr;
    bool path_started = false;
    Vec2 path_start = VEC2_ZERO;
    Color32 color;
    struct {
        Vec2 verts[RASTER_MAX_VERTICES];
        float vy[RASTER_MAX_VERTICES];
        int vertex_count = 0;
    } polygon;
    Bounds2 bounds;
    noz::RectInt clip;
};

Rasterizer* CreateRasterizer(Allocator* allocator, PixelData* pixels) {
    RasterizerImpl* impl = static_cast<RasterizerImpl*>(Alloc(allocator, sizeof(RasterizerImpl)));
    impl->pixels = pixels;
    impl->clip = { 0, 0, pixels->size.x, pixels->size.y };
    return impl;
}

void SetClipRect(Rasterizer* rasterizer, int x, int y, int w, int h) {
    RasterizerImpl* impl = static_cast<RasterizerImpl*>(rasterizer);
    impl->clip = {
        Max(0,x),
        Max(0,y),
        Min(impl->pixels->size.x - x, w),
        Min(impl->pixels->size.y - y, h)
    };
}

void SetColor(Rasterizer* rasterizer, const Color& color) {
    RasterizerImpl* impl = static_cast<RasterizerImpl*>(rasterizer);
    impl->color = ColorToColor32(color);
}

void BeginPath(Rasterizer* rasterizer) {
    RasterizerImpl* impl = static_cast<RasterizerImpl*>(rasterizer);
    impl->polygon.vertex_count = 0;
    impl->path_started = false;
    impl->bounds.min.x = FLT_MAX;
    impl->bounds.min.y = FLT_MAX;
    impl->bounds.max.x = FLT_MIN;
    impl->bounds.max.y = FLT_MIN;
}

inline void AddVertex(RasterizerImpl* impl, float x, float y) {
    if (impl->polygon.vertex_count >= RASTER_MAX_VERTICES) return;
    impl->polygon.verts[impl->polygon.vertex_count++] = {x,y};
}

void MoveTo(Rasterizer* rasterizer, float x, float y) {
    RasterizerImpl* impl = static_cast<RasterizerImpl*>(rasterizer);
    impl->path_start = {x, y};
    impl->path_started = true;
    AddVertex(impl, x, y);
    impl->bounds.min.x = std::min(impl->bounds.min.x, x);
    impl->bounds.min.y = std::min(impl->bounds.min.y, y);
    impl->bounds.max.x = std::max(impl->bounds.max.x, x);
    impl->bounds.max.y = std::max(impl->bounds.max.y, y);
}

void LineTo(Rasterizer* rasterizer, float x, float y) {
    RasterizerImpl* impl = static_cast<RasterizerImpl*>(rasterizer);
    assert(impl->path_started && "LineTo called before MoveTo");

    AddVertex(impl, x, y);
    impl->bounds.min.x = std::min(impl->bounds.min.x, x);
    impl->bounds.min.y = std::min(impl->bounds.min.y, y);
    impl->bounds.max.x = std::max(impl->bounds.max.x, x);
    impl->bounds.max.y = std::max(impl->bounds.max.y, y);
}

void CurveTo(Rasterizer* rasterizer, float x1, float y1, float x2, float y2, float x3, float y3) {
    RasterizerImpl* impl = static_cast<RasterizerImpl*>(rasterizer);
    assert(impl->path_started && "LineTo called before MoveTo");
    const Vec2 pos = impl->polygon.verts[impl->polygon.vertex_count-1];

    constexpr int segments = 16;
    for (int i = 1; i <= segments; i++) {
        float t = (float)i / segments;
        float t2 = t * t;
        float t3 = t2 * t;
        float mt = 1.0f - t;
        float mt2 = mt * mt;
        float mt3 = mt2 * mt;

        float x = mt3 * pos.x + 3.0f * mt2 * t * x1 + 3.0f * mt * t2 * x2 + t3 * x3;
        float y = mt3 * pos.y + 3.0f * mt2 * t * y1 + 3.0f * mt * t2 * y2 + t3 * y3;
        LineTo(rasterizer, x, y);
    }
}

inline void BlendPixel(Color32* dst, const Color32& color) {
    if (color.a == 0) return;

    if (dst->a == 0 || color.a == 255)
        *dst = color;
    else
        *dst = Blend(*dst, color);
}


static bool IsInPath(RasterizerImpl* impl, const Vec2& p) {
    const int vertex_count = impl->polygon.vertex_count;
    Vec2* verts = impl->polygon.verts;

    // Use winding number algorithm with top-left fill rule
    // This ensures pixels on shared edges are claimed by exactly one face
    int winding = 0;
    for (int i = 0; i < vertex_count; i++) {
        int j = (i + 1) % vertex_count;
        const Vec2& p0 = verts[i];
        const Vec2& p1 = verts[j];

        // upward crossing (p0.y <= p.y < p1.y)
        if (p0.y <= p.y) {
            if (p1.y > p.y) {
                float cross = (p1.x - p0.x) * (p.y - p0.y) - (p.x - p0.x) * (p1.y - p0.y);
                // >= 0 to include points exactly on left/upward edges
                if (cross >= 0) winding++;
            }
        }
        // downward crossing (p1.y <= p.y < p0.y)
        else if (p1.y <= p.y) {
            float cross = (p1.x - p0.x) * (p.y - p0.y) - (p.x - p0.x) * (p1.y - p0.y);
            // < 0 (strict) to exclude points exactly on right/downward edges
            if (cross < 0) winding--;
        }
    }
    return winding != 0;
}

void Fill(Rasterizer* rasterizer, const Vec2Int& offset) {
    RasterizerImpl* impl = static_cast<RasterizerImpl*>(rasterizer);

    if (!impl->pixels) return;
    if (impl->polygon.vertex_count < 3) return;

    int x_min = impl->clip.x;
    int y_min = impl->clip.y;
    int x_max = impl->clip.x + impl->clip.w;
    int y_max = impl->clip.y + impl->clip.h;

    const Color32& color = impl->color;

    for (int y = y_min; y < y_max; y++) {
        Color32* row = impl->pixels->rgba + offset.x + (y + offset.y) * impl->pixels->size.x;
        float py = y + 0.5f;

        for (int x = x_min; x < x_max; x++)
            if (IsInPath(impl, Vec2{x + 0.5f, py}))
                BlendPixel(row + x, color);
    }

    impl->polygon.vertex_count = 0;
}
