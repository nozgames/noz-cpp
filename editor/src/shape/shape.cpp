//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "shape.h"
#include "../utils/pixel_data.h"

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
            a0->midpoint = (p0 + p1) * 0.5f;
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
            a0->midpoint = (p0 * 0.25f) + (cp * 0.5f) + (p1 * 0.25f);
        }
    }

    bool HitTest(Shape* shape, const Vec2& point, HitResult* result) {
        const float anchor_radius = noz::editor::g_workspace.select_size;
        const float anchor_radius_sqr = anchor_radius * anchor_radius;
        const float segment_radius_sqr = noz::editor::g_workspace.select_size * noz::editor::g_workspace.select_size;
        const float midpoint_radius_sqr = anchor_radius_sqr * 0.5f;

        result->anchor_index = U16_MAX;
        result->segment_index = U16_MAX;
        result->midpoint_index = U16_MAX;
        result->path_index = U16_MAX;
        result->anchor_dist_sqr = FLT_MAX;
        result->segment_dist_sqr = FLT_MAX;
        result->midpoint_dist_sqr = FLT_MAX;

        float best_anchor_dist_sqr = anchor_radius_sqr;
        float best_segment_dist_sqr = segment_radius_sqr;
        float best_midpoint_dist_sqr = midpoint_radius_sqr;
        u16 best_anchor_index = U16_MAX;
        u16 best_anchor_path = U16_MAX;
        u16 best_segment_index = U16_MAX;
        u16 best_segment_path = U16_MAX;
        u16 best_midpoint_index = U16_MAX;
        u16 best_midpoint_path = U16_MAX;
        u16 hit_path_index = U16_MAX;

        for (u16 p_idx = 0; p_idx < shape->path_count; ++p_idx) {
            Path& p = shape->paths[p_idx];

            // Check anchors - find closest one
            for (u16 a_idx = 0; a_idx < p.anchor_count; ++a_idx) {
                const Anchor* a = GetAnchor(shape, &p, a_idx);
                float dist_sqr = LengthSqr(point - a->position);
                if (dist_sqr < best_anchor_dist_sqr) {
                    best_anchor_dist_sqr = dist_sqr;
                    best_anchor_index = p.anchor_start + a_idx;
                    best_anchor_path = p_idx;
                }
            }

            // Check segments - find closest one
            for (u16 a_idx = 0; a_idx < p.anchor_count; ++a_idx) {
                const Anchor* a0 = GetAnchor(shape, &p, a_idx);
                u16 next_idx = (a_idx + 1) % p.anchor_count;
                const Anchor* a1 = GetAnchor(shape, &p, next_idx);

                auto test_edge = [&](const Vec2& ea, const Vec2& eb) {
                    Vec2 edge_dir = eb - ea;
                    float edge_length = Length(edge_dir);
                    if (edge_length < FLT_EPSILON) return;
                    edge_dir = edge_dir / edge_length;

                    Vec2 to_point = point - ea;
                    float proj = Dot(to_point, edge_dir);
                    if (proj > 0.0f && proj < edge_length) {
                        Vec2 closest = ea + edge_dir * proj;
                        float dist_sqr = LengthSqr(point - closest);
                        if (dist_sqr < best_segment_dist_sqr) {
                            best_segment_dist_sqr = dist_sqr;
                            best_segment_index = p.anchor_start + a_idx;
                            best_segment_path = p_idx;
                        }
                    }
                };

                Vec2 v0 = a0->position;
                for (int si = 0; si < SHAPE_MAX_SEGMENT_SAMPLES; ++si) {
                    Vec2 v1 = a0->samples[si];
                    test_edge(v0, v1);
                    v0 = v1;
                }
                test_edge(v0, a1->position);

                float midpoint_dist_sqr = LengthSqr(point - a0->midpoint);
                if (midpoint_dist_sqr < best_midpoint_dist_sqr) {
                    best_midpoint_dist_sqr = midpoint_dist_sqr;
                    best_midpoint_index = p.anchor_start + a_idx;
                    best_midpoint_path = p_idx;
                }
            }

            // Check filled path
            if (hit_path_index == U16_MAX) {
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
                        hit_path_index = p_idx;
                    }
                }
            }
        }

        if (best_anchor_index != U16_MAX) {
            result->anchor_index = best_anchor_index;
            result->anchor_dist_sqr = best_anchor_dist_sqr;
            result->path_index = best_anchor_path;
        }
        if (best_midpoint_index != U16_MAX) {
            result->midpoint_index = best_midpoint_index;
            result->midpoint_dist_sqr = best_midpoint_dist_sqr;
            if (result->anchor_index == U16_MAX) {
                result->path_index = best_midpoint_path;
            }
        }
        if (best_segment_index != U16_MAX) {
            result->segment_index = best_segment_index;
            result->segment_dist_sqr = best_segment_dist_sqr;
            if (result->anchor_index == U16_MAX && result->midpoint_index == U16_MAX) {
                result->path_index = best_segment_path;
            }
        }
        if (hit_path_index != U16_MAX && result->anchor_index == U16_MAX && result->midpoint_index == U16_MAX && result->segment_index == U16_MAX) {
            result->path_index = hit_path_index;
        }

        return result->anchor_index != U16_MAX || result->midpoint_index != U16_MAX || result->segment_index != U16_MAX || result->path_index != U16_MAX;
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
                    // Exclude endpoints - anchors should be hit tested separately
                    if (proj > 0.0f && proj < edge_length) {
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

    u16 InsertAnchor(Shape* shape, u16 after_anchor_index, const Vec2& position, float curve) {
        if (shape->anchor_count >= SHAPE_MAX_ANCHORS)
            return U16_MAX;

        u16 path_idx = U16_MAX;
        u16 local_idx = U16_MAX;
        for (u16 p = 0; p < shape->path_count; ++p) {
            Path* path = &shape->paths[p];
            if (after_anchor_index >= path->anchor_start && after_anchor_index < path->anchor_start + path->anchor_count) {
                path_idx = p;
                local_idx = after_anchor_index - path->anchor_start;
                break;
            }
        }
        if (path_idx == U16_MAX)
            return U16_MAX;

        u16 insert_pos = after_anchor_index + 1;

        for (u16 i = shape->anchor_count; i > insert_pos; --i) {
            shape->anchors[i] = shape->anchors[i - 1];
        }

        Anchor* new_anchor = &shape->anchors[insert_pos];
        *new_anchor = {};
        new_anchor->position = position;
        new_anchor->curve = curve;
        new_anchor->flags = ANCHOR_FLAG_NONE;

        shape->anchor_count++;
        shape->paths[path_idx].anchor_count++;

        for (u16 p = path_idx + 1; p < shape->path_count; ++p) {
            shape->paths[p].anchor_start++;
        }

        UpdateSamples(shape, path_idx, local_idx);
        UpdateSamples(shape, path_idx, local_idx + 1);

        return insert_pos;
    }

    u16 SplitSegment(Shape* shape, u16 anchor_index) {
        if (shape->anchor_count >= SHAPE_MAX_ANCHORS)
            return U16_MAX;

        u16 path_idx = U16_MAX;
        u16 local_idx = 0;
        for (u16 p = 0; p < shape->path_count; ++p) {
            Path* path = &shape->paths[p];
            if (anchor_index >= path->anchor_start && anchor_index < path->anchor_start + path->anchor_count) {
                path_idx = p;
                local_idx = anchor_index - path->anchor_start;
                break;
            }
        }
        if (path_idx == U16_MAX)
            return U16_MAX;

        Path* path = &shape->paths[path_idx];
        Anchor* a0 = &shape->anchors[anchor_index];
        Anchor* a1 = GetAnchor(shape, path, (local_idx + 1) % path->anchor_count);

        Vec2 p0 = a0->position;
        Vec2 p1 = a1->position;
        Vec2 midpoint_pos = a0->midpoint;
        float original_curve = a0->curve;

        float curve0 = 0.0f;
        float curve1 = 0.0f;

        if (Abs(original_curve) > FLT_EPSILON) {
            Vec2 chord_mid = (p0 + p1) * 0.5f;
            Vec2 dir = p1 - p0;
            Vec2 perp = Normalize(Vec2{-dir.y, dir.x});
            Vec2 cp = chord_mid + perp * original_curve;

            Vec2 cp0 = (p0 + cp) * 0.5f;
            Vec2 cp1 = (cp + p1) * 0.5f;

            Vec2 mid0 = (p0 + midpoint_pos) * 0.5f;
            Vec2 dir0 = midpoint_pos - p0;
            Vec2 perp0 = Normalize(Vec2{-dir0.y, dir0.x});
            curve0 = Dot(cp0 - mid0, perp0);

            Vec2 mid1 = (midpoint_pos + p1) * 0.5f;
            Vec2 dir1 = p1 - midpoint_pos;
            Vec2 perp1 = Normalize(Vec2{-dir1.y, dir1.x});
            curve1 = Dot(cp1 - mid1, perp1);
        }

        u16 new_idx = InsertAnchor(shape, anchor_index, midpoint_pos, 0.0f);
        if (new_idx != U16_MAX) {
            shape->anchors[anchor_index].curve = curve0;
            shape->anchors[new_idx].curve = curve1;
            UpdateSamples(shape);
        }

        return new_idx;
    }

    void DeleteSelectedAnchors(Shape* shape) {
        for (u16 p_idx = 0; p_idx < shape->path_count; ) {
            Path* path = &shape->paths[p_idx];

            u16 keep_count = 0;
            for (u16 a_idx = 0; a_idx < path->anchor_count; ++a_idx) {
                Anchor* a = GetAnchor(shape, path, a_idx);
                if (!IsSelected(a))
                    keep_count++;
            }

            if (keep_count == 0) {
                u16 anchors_to_remove = path->anchor_count;
                u16 anchor_start = path->anchor_start;

                for (u16 i = anchor_start; i < shape->anchor_count - anchors_to_remove; ++i) {
                    shape->anchors[i] = shape->anchors[i + anchors_to_remove];
                }
                shape->anchor_count -= anchors_to_remove;

                for (u16 i = p_idx; i < shape->path_count - 1; ++i) {
                    shape->paths[i] = shape->paths[i + 1];
                    shape->paths[i].anchor_start -= anchors_to_remove;
                }
                shape->path_count--;
            } else if (keep_count < path->anchor_count) {
                u16 write_idx = path->anchor_start;
                u16 removed = 0;

                for (u16 a_idx = 0; a_idx < path->anchor_count; ++a_idx) {
                    u16 read_idx = path->anchor_start + a_idx;
                    Anchor* a = &shape->anchors[read_idx];

                    if (!IsSelected(a)) {
                        if (write_idx != read_idx) {
                            shape->anchors[write_idx] = shape->anchors[read_idx];
                        }
                        write_idx++;
                    } else {
                        removed++;
                    }
                }

                u16 old_end = path->anchor_start + path->anchor_count;
                for (u16 i = write_idx; i < shape->anchor_count - removed; ++i) {
                    shape->anchors[i] = shape->anchors[i + removed];
                }

                shape->anchor_count -= removed;
                path->anchor_count = keep_count;

                for (u16 i = p_idx + 1; i < shape->path_count; ++i) {
                    shape->paths[i].anchor_start -= removed;
                }

                p_idx++;
            } else {
                p_idx++;
            }
        }

        UpdateSamples(shape);
    }
}
