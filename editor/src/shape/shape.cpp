//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "shape.h"
#include "../utils/pixel_data.h"
#include <vector>

namespace noz::editor::shape {

    static bool IsInPolygon(const Vec2* verts, int vertex_count, const Vec2& p) {
        int winding = 0;
        for (int i = 0; i < vertex_count; i++) {
            int j = (i + 1) % vertex_count;
            const Vec2& p0 = verts[i];
            const Vec2& p1 = verts[j];

            if (p0.y <= p.y) {
                if (p1.y > p.y) {
                    float cross = (p1.x - p0.x) * (p.y - p0.y) - (p.x - p0.x) * (p1.y - p0.y);
                    if (cross >= 0) winding++;
                }
            } else if (p1.y <= p.y) {
                float cross = (p1.x - p0.x) * (p.y - p0.y) - (p.x - p0.x) * (p1.y - p0.y);
                if (cross < 0) winding--;
            }
        }
        return winding != 0;
    }

    void UpdateSamples(Shape* shape) {
        for (u16 p_idx = 0; p_idx < shape->path_count; ++p_idx) {
            Path* path = &shape->paths[p_idx];
            for (u16 a_idx = 0; a_idx < path->anchor_count; ++a_idx) {
                UpdateSamples(shape, p_idx, a_idx);
            }
        }
    }

    void UpdateSamples(Shape* shape, u16 path_idx, u16 anchor_idx) {
        Path* path = &shape->paths[path_idx];
        Anchor* a0 = GetAnchor(shape, path, anchor_idx);
        u16 next_idx = (anchor_idx + 1) % path->anchor_count;
        Anchor* a1 = GetAnchor(shape, path, next_idx);

        Vec2 p0 = a0->position;
        Vec2 p1 = a1->position;

        if (Abs(a0->curve) < FLT_EPSILON) {
            for (int i = 0; i < SHAPE_MAX_SEGMENT_SAMPLES; ++i) {
                float t = static_cast<float>(i + 1) / static_cast<float>(SHAPE_MAX_SEGMENT_SAMPLES + 1);
                a0->samples[i] = p0 + (p1 - p0) * t;
            }
        } else {
            Vec2 mid = (p0 + p1) * 0.5f;
            Vec2 dir = p1 - p0;
            Vec2 perp = Normalize(Vec2{-dir.y, dir.x});
            Vec2 cp = mid + perp * a0->curve;

            for (int i = 0; i < SHAPE_MAX_SEGMENT_SAMPLES; ++i) {
                float t = static_cast<float>(i + 1) / static_cast<float>(SHAPE_MAX_SEGMENT_SAMPLES + 1);
                float u = 1.0f - t;
                a0->samples[i] = (p0 * (u * u)) + (cp * (2.0f * u * t)) + (p1 * (t * t));
            }
        }
    }

    bool HitTest(Shape* shape, const Vec2& point, HitResult* result) {
        const float anchor_screen_px = 10.0f;
        float anchor_radius_world = Abs((noz::ScreenToWorld(noz::editor::g_workspace.camera, Vec2{0, anchor_screen_px}) - noz::ScreenToWorld(noz::editor::g_workspace.camera, VEC2_ZERO)).y);
        const float anchor_radius_sqr = anchor_radius_world * anchor_radius_world;
        const float segment_radius_sqr = noz::editor::g_workspace.select_size * noz::editor::g_workspace.select_size;

        result->anchor_index = U16_MAX;
        result->segment_index = U16_MAX;
        result->path_index = U16_MAX;

        for (u16 p_idx = 0; p_idx < shape->path_count; ++p_idx) {
            Path& p = shape->paths[p_idx];

            // Check anchors first
            for (u16 a_idx = 0; a_idx < p.anchor_count; ++a_idx) {
                const Anchor* a = GetAnchor(shape, &p, a_idx);
                float dist_sqr = LengthSqr(point - a->position);
                if (dist_sqr <= anchor_radius_sqr) {
                    result->anchor_index = p.anchor_start + a_idx;
                    result->path_index = p_idx;
                    return true;
                }
            }

            // Check segments (edges between anchors)
            for (u16 a_idx = 0; a_idx < p.anchor_count; ++a_idx) {
                const Anchor* a0 = GetAnchor(shape, &p, a_idx);
                u16 next_idx = (a_idx + 1) % p.anchor_count;
                const Anchor* a1 = GetAnchor(shape, &p, next_idx);

                auto test_edge = [&](const Vec2& ea, const Vec2& eb) -> bool {
                    Vec2 edge_dir = eb - ea;
                    float edge_length = Length(edge_dir);
                    if (edge_length < FLT_EPSILON) return false;
                    edge_dir = Normalize(edge_dir);

                    Vec2 to_point = point - ea;
                    float proj = Dot(to_point, edge_dir);
                    if (proj >= 0.0f && proj <= edge_length) {
                        Vec2 closest = ea + edge_dir * proj;
                        float dist_sqr = LengthSqr(point - closest);
                        if (dist_sqr <= segment_radius_sqr) return true;
                    }
                    return false;
                };

                // Test from anchor to first sample, through samples, to next anchor
                Vec2 v0 = a0->position;
                for (int si = 0; si < SHAPE_MAX_SEGMENT_SAMPLES; ++si) {
                    Vec2 v1 = a0->samples[si];
                    if (test_edge(v0, v1)) {
                        result->segment_index = p.anchor_start + a_idx;
                        result->path_index = p_idx;
                        return true;
                    }
                    v0 = v1;
                }
                // Last segment to next anchor
                if (test_edge(v0, a1->position)) {
                    result->segment_index = p.anchor_start + a_idx;
                    result->path_index = p_idx;
                    return true;
                }
            }

            // Check filled path
            std::vector<Vec2> verts;
            verts.reserve(p.anchor_count * (SHAPE_MAX_SEGMENT_SAMPLES + 2));

            for (u16 a_idx = 0; a_idx < p.anchor_count; ++a_idx) {
                const Anchor* a = GetAnchor(shape, &p, a_idx);
                verts.push_back(a->position);
                for (int k = 0; k < SHAPE_MAX_SEGMENT_SAMPLES; ++k) {
                    verts.push_back(a->samples[k]);
                }
            }

            if (verts.size() >= 3) {
                int winding = 0;
                for (size_t i = 0; i < verts.size(); ++i) {
                    size_t j = (i + 1) % verts.size();
                    const Vec2& p0 = verts[i];
                    const Vec2& p1 = verts[j];

                    if (p0.y <= point.y) {
                        if (p1.y > point.y) {
                            float cross = (p1.x - p0.x) * (point.y - p0.y) - (point.x - p0.x) * (p1.y - p0.y);
                            if (cross >= 0) winding++;
                        }
                    } else if (p1.y <= point.y) {
                        float cross = (p1.x - p0.x) * (point.y - p0.y) - (point.x - p0.x) * (p1.y - p0.y);
                        if (cross < 0) winding--;
                    }
                }

                if (winding != 0) {
                    result->path_index = p_idx;
                    return true;
                }
            }
        }

        return false;
    }

    u16 HitTestAll(Shape* shape, const Vec2& point, HitResult* results, u16 max_results) {
        if (max_results == 0) return 0;

        u16 count = 0;

        const float anchor_screen_px = 10.0f;
        float anchor_radius_world = Abs((noz::ScreenToWorld(noz::editor::g_workspace.camera, Vec2{0, anchor_screen_px}) - noz::ScreenToWorld(noz::editor::g_workspace.camera, VEC2_ZERO)).y);
        const float anchor_radius_sqr = anchor_radius_world * anchor_radius_world;
        const float segment_radius_sqr = noz::editor::g_workspace.select_size * noz::editor::g_workspace.select_size;

        for (u16 p_idx = 0; p_idx < shape->path_count && count < max_results; ++p_idx) {
            Path& p = shape->paths[p_idx];

            // Check anchors
            for (u16 a_idx = 0; a_idx < p.anchor_count && count < max_results; ++a_idx) {
                const Anchor* a = GetAnchor(shape, &p, a_idx);
                float dist_sqr = LengthSqr(point - a->position);
                if (dist_sqr <= anchor_radius_sqr) {
                    results[count] = {};
                    results[count].anchor_index = p.anchor_start + a_idx;
                    results[count].path_index = p_idx;
                    count++;
                }
            }

            // Check segments
            for (u16 a_idx = 0; a_idx < p.anchor_count && count < max_results; ++a_idx) {
                const Anchor* a0 = GetAnchor(shape, &p, a_idx);
                u16 next_idx = (a_idx + 1) % p.anchor_count;
                const Anchor* a1 = GetAnchor(shape, &p, next_idx);

                auto test_edge = [&](const Vec2& ea, const Vec2& eb) -> bool {
                    Vec2 edge_dir = eb - ea;
                    float edge_length = Length(edge_dir);
                    if (edge_length < FLT_EPSILON) return false;
                    edge_dir = Normalize(edge_dir);

                    Vec2 to_point = point - ea;
                    float proj = Dot(to_point, edge_dir);
                    if (proj >= 0.0f && proj <= edge_length) {
                        Vec2 closest = ea + edge_dir * proj;
                        float dist_sqr = LengthSqr(point - closest);
                        if (dist_sqr <= segment_radius_sqr) return true;
                    }
                    return false;
                };

                bool hit = false;
                Vec2 v0 = a0->position;
                for (int si = 0; si < SHAPE_MAX_SEGMENT_SAMPLES && !hit; ++si) {
                    Vec2 v1 = a0->samples[si];
                    hit = test_edge(v0, v1);
                    v0 = v1;
                }
                if (!hit) {
                    hit = test_edge(v0, a1->position);
                }

                if (hit) {
                    results[count] = {};
                    results[count].segment_index = p.anchor_start + a_idx;
                    results[count].path_index = p_idx;
                    count++;
                }
            }

            // Check filled path
            if (count < max_results) {
                std::vector<Vec2> verts;
                verts.reserve(p.anchor_count * (SHAPE_MAX_SEGMENT_SAMPLES + 2));

                for (u16 a_idx = 0; a_idx < p.anchor_count; ++a_idx) {
                    const Anchor* a = GetAnchor(shape, &p, a_idx);
                    verts.push_back(a->position);
                    for (int k = 0; k < SHAPE_MAX_SEGMENT_SAMPLES; ++k) {
                        verts.push_back(a->samples[k]);
                    }
                }

                if (verts.size() >= 3) {
                    int winding = 0;
                    for (size_t i = 0; i < verts.size(); ++i) {
                        size_t j = (i + 1) % verts.size();
                        const Vec2& p0 = verts[i];
                        const Vec2& p1 = verts[j];

                        if (p0.y <= point.y) {
                            if (p1.y > point.y) {
                                float cross = (p1.x - p0.x) * (point.y - p0.y) - (point.x - p0.x) * (p1.y - p0.y);
                                if (cross >= 0) winding++;
                            }
                        } else if (p1.y <= point.y) {
                            float cross = (p1.x - p0.x) * (point.y - p0.y) - (point.x - p0.x) * (p1.y - p0.y);
                            if (cross < 0) winding--;
                        }
                    }

                    if (winding != 0) {
                        results[count] = {};
                        results[count].path_index = p_idx;
                        count++;
                    }
                }
            }
        }

        return count;
    }

    void ClearSelection(Shape* shape) {
        for (u16 p_idx = 0; p_idx < shape->path_count; ++p_idx) {
            Path& p = shape->paths[p_idx];
            ClearFlags(&p, PATH_FLAG_SELECTED);
            for (u16 a_idx = 0; a_idx < p.anchor_count; ++a_idx) {
                ClearFlags(GetAnchor(shape, &p, a_idx), ANCHOR_FLAG_SELECTED);
            }
        }
    }

    void UpdateBounds(Shape* shape) {
        if (shape->anchor_count == 0) {
            shape->bounds = { VEC2_ZERO, VEC2_ZERO };
            shape->raster_bounds = { 0, 0, 0, 0 };
            return;
        }

        Vec2 min_pt = shape->anchors[0].position;
        Vec2 max_pt = shape->anchors[0].position;

        for (u16 i = 0; i < shape->anchor_count; ++i) {
            Anchor* anchor = &shape->anchors[i];
            min_pt = Min(min_pt, anchor->position);
            max_pt = Max(max_pt, anchor->position);
            for (int s = 0; s < SHAPE_MAX_SEGMENT_SAMPLES; ++s) {
                min_pt = Min(min_pt, anchor->samples[s]);
                max_pt = Max(max_pt, anchor->samples[s]);
            }
        }

        shape->bounds = { min_pt, max_pt };

        float dpi = (float)g_editor.atlas.dpi;
        int x_min = FloorToInt(min_pt.x * dpi);
        int y_min = FloorToInt(min_pt.y * dpi);
        int x_max = CeilToInt(max_pt.x * dpi);
        int y_max = CeilToInt(max_pt.y * dpi);

        shape->raster_bounds = { x_min, y_min, x_max - x_min, y_max - y_min };
    }

    void Rasterize(Shape* shape, PixelData* pixels, const Color* palette, const Vec2Int& offset) {
        if (shape->path_count == 0) return;

        float dpi = (float)g_editor.atlas.dpi;

        // Temporary vertex buffer for polygon rasterization
        constexpr int MAX_POLY_VERTS = 256;
        Vec2 poly_verts[MAX_POLY_VERTS];

        for (u16 p_idx = 0; p_idx < shape->path_count; ++p_idx) {
            Path* path = &shape->paths[p_idx];
            if (path->anchor_count < 3) continue;

            // Build polygon vertices in pixel space
            // Each anchor's samples are the interpolated points AFTER that anchor (toward the next)
            int vertex_count = 0;
            for (u16 a_idx = 0; a_idx < path->anchor_count && vertex_count < MAX_POLY_VERTS; ++a_idx) {
                Anchor* anchor = GetAnchor(shape, path, a_idx);

                // Add anchor position
                poly_verts[vertex_count++] = anchor->position * dpi;

                // Only add samples if curve is non-zero (curved segment)
                if (Abs(anchor->curve) > FLT_EPSILON) {
                    for (int s = 0; s < SHAPE_MAX_SEGMENT_SAMPLES && vertex_count < MAX_POLY_VERTS; ++s) {
                        poly_verts[vertex_count++] = anchor->samples[s] * dpi;
                    }
                }
            }

            if (vertex_count < 3) continue;

            Color fill_color = palette[path->fill_color];
            Color32 color32 = ColorToColor32(fill_color);

            // Rasterize within raster_bounds
            // offset maps raster_bounds origin to texture origin
            RectInt& rb = shape->raster_bounds;
            for (int y = 0; y < rb.h; ++y) {
                int py = offset.y + rb.y + y;
                if (py < 0 || py >= pixels->size.y) continue;

                Color32* row = pixels->rgba + py * pixels->size.x;
                float sample_y = rb.y + y + 0.5f;

                for (int x = 0; x < rb.w; ++x) {
                    int px = offset.x + rb.x + x;
                    if (px < 0 || px >= pixels->size.x) continue;

                    float sample_x = rb.x + x + 0.5f;
                    if (IsInPolygon(poly_verts, vertex_count, Vec2{sample_x, sample_y})) {
                        Color32* dst = row + px;
                        if (color32.a == 255 || dst->a == 0) {
                            *dst = color32;
                        } else if (color32.a > 0) {
                            *dst = Blend(*dst, color32);
                        }
                    }
                }
            }
        }
    }
}
