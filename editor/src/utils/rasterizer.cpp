//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "rasterizer.h"

constexpr int RASTER_MAX_VERTICES = 256;

struct RasterizerImpl : Rasterizer {
    u8* pixels = nullptr;
    bool path_started = false;
    Vec2 path_start = VEC2_ZERO;
    Vec2Int size = VEC2INT_ZERO;
    Color32 color;
    struct {
        Vec2 verts[RASTER_MAX_VERTICES];
        float vy[RASTER_MAX_VERTICES];
        int vertex_count = 0;
    } polygon;
    Bounds2 bounds;
    noz::RectInt clip;
};

Rasterizer* CreateRasterizer(Allocator* allocator) {
    Rasterizer* rasterizer = static_cast<Rasterizer*>(Alloc(allocator, sizeof(RasterizerImpl)));
    return rasterizer;
}

void SetTarget(Rasterizer* rasterizer, u8* pixels, int width, int height) {
    RasterizerImpl* impl = static_cast<RasterizerImpl*>(rasterizer);
    impl->pixels = pixels;
    impl->size.x = width;
    impl->size.y = height;
    impl->clip = { 0, 0, width, height };
}

void SetClipRect(Rasterizer* rasterizer, int x, int y, int w, int h) {
    RasterizerImpl* impl = static_cast<RasterizerImpl*>(rasterizer);
    impl->clip = { x, y, w, h };
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

    // Center
    Vec2* verts = impl->polygon.verts;
    Vec2 c = VEC2_ZERO;
    for (int i = 0; i < vertex_count; i++)
        c += verts[i];

    c /= (float)vertex_count;

    // Slightly move test point toward center (makes test more inclusive)
    constexpr float BIAS = 0.01f;
    Vec2 bp = p + (c - p) * BIAS;

    int winding = 0;
    for (int i = 0; i < vertex_count; i++) {
        int j = (i + 1) % vertex_count;
        const Vec2& p0 = verts[i];
        const Vec2& p1 = verts[j];

        if (p0.y <= bp.y) {
            if (p1.y > bp.y) {
                float cross = (p1.x - p0.x) * (bp.y - p0.y) - (bp.x - p0.x) * (p1.y - p0.y);
                if (cross > 0) winding++;
            }
        } else if (p1.y <= bp.y) {
            float cross = (p1.x - p0.x) * (bp.y - p0.y) - (bp.x - p0.x) * (p1.y - p0.y);
            if (cross < 0) winding--;
        }
    }
    return winding != 0;
}

void Fill(Rasterizer* rasterizer) {
    RasterizerImpl* impl = static_cast<RasterizerImpl*>(rasterizer);

    if (!impl->pixels) return;
    if (impl->polygon.vertex_count < 3) return;

    int x_min = Max(FloorToInt(impl->bounds.min.x), impl->clip.x);
    int y_min = Max(FloorToInt(impl->bounds.min.y), impl->clip.y);
    int x_max = Max(CeilToInt(impl->bounds.max.x), impl->clip.x + impl->clip.w);
    int y_max = Max(CeilToInt(impl->bounds.max.y), impl->clip.y + impl->clip.h);

    const Color32& color = impl->color;

    for (int y = y_min; y < y_max; y++) {
        Color32* row = reinterpret_cast<Color32*>(impl->pixels + y * impl->size.x * 4);
        float py = y + 0.5f;

        for (int x = x_min; x < x_max; x++)
            if (IsInPath(impl, Vec2{x + 0.5f, py}))
                BlendPixel(row + x, color);
    }

    impl->polygon.vertex_count = 0;
}
