//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "shape.h"
#include <vector>

namespace noz::editor::shape {

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

        for (int i = 0; i < SHAPE_MAX_SEGMENT_SAMPLES; ++i) {
            float t = static_cast<float>(i + 1) / static_cast<float>(SHAPE_MAX_SEGMENT_SAMPLES + 1);
            a0->samples[i] = p0 + (p1 - p0) * t;
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
}
