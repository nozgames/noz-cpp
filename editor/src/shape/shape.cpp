//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "shape.h"
#include <vector>

namespace noz::editor::shape {

    void UpdateSegmentSamples(Shape* shape, u16 s_idx) {
        Segment* s = &shape->segments[s_idx];
        if (LengthSqr(s->curve.offset) < FLT_EPSILON) {
            s->sample_count = 0;
            return;
        }
        
        Vec2 p0 = shape->anchors[s->anchor0].position;
        Vec2 p1 = shape->anchors[s->anchor1].position;
        Vec2 cp = (p0 + p1) * 0.5f + s->curve.offset;
        float w = s->curve.weight;
        const int num_samples = SHAPE_MAX_SEGMENT_SAMPLES;
        for (int i = 0; i < num_samples; ++i) {
            float t = static_cast<float>(i + 1) / static_cast<float>(num_samples + 1);
            float u = 1.0f - t;
            Vec2 point = (p0 * (u * u)) + (cp * (2.0f * u * t)) + (p1 * (t * t));
            s->samples[i] = point;
        }

        s->sample_count = num_samples;
    }

    bool HitTest(Shape* shape, HitResult* result) {
        // Delegate to the point-based overload using the current mouse world position
        return HitTest(shape, noz::editor::g_workspace.mouse_world_position, result);
    }

    bool HitTest(Shape* shape, const Vec2& point, HitResult* result) {
        // Compute hit radii such that they are constant on screen regardless of zoom.
        // - Anchors should be small so they don't dominate hits; use ~10 screen pixels
        // - Segments use the workspace select size (larger)
        const float anchor_screen_px = 10.0f; // px
        float anchor_radius_world = Abs((noz::ScreenToWorld(noz::editor::g_workspace.camera, Vec2{0, anchor_screen_px}) - noz::ScreenToWorld(noz::editor::g_workspace.camera, VEC2_ZERO)).y);
        const float anchor_radius_sqr = anchor_radius_world * anchor_radius_world;

        const float segment_radius_sqr = noz::editor::g_workspace.select_size * noz::editor::g_workspace.select_size;

        // Initialize result to no-hit
        result->anchor_index = U16_MAX;
        result->segment_index = U16_MAX;
        result->path_index = U16_MAX;

        // Check anchors first (small screen-constant radius)
        for (u16 a_idx = 0; a_idx < shape->anchor_count; ++a_idx) {
            const Anchor& a = shape->anchors[a_idx];
            if (!IsActive(&a)) continue;
            float dist_sqr = LengthSqr(point - a.position);
            if (dist_sqr <= anchor_radius_sqr) {
                result->anchor_index = a_idx;
                return true;
            }
        }

        // Check segments (edges / sampled curves)
        for (u16 s_idx = 0; s_idx < shape->segment_count; ++s_idx) {
            Segment& s = shape->segments[s_idx];
            if (!IsActive(&s)) continue;

            // Ensure samples are up to date
            UpdateSegmentSamples(shape, s_idx);

            // Build a list of points making up the segment (anchor0 -> samples... -> anchor1)
            Vec2 v0 = shape->anchors[s.anchor0].position;

            auto test_edge = [&](const Vec2& a, const Vec2& b)->bool {
                Vec2 edge_dir = b - a;
                float edge_length = Length(edge_dir);
                if (edge_length < FLT_EPSILON) return false;
                edge_dir = Normalize(edge_dir);

                Vec2 to_point = point - a;
                float proj = Dot(to_point, edge_dir);
                if (proj >= 0.0f && proj <= edge_length) {
                    Vec2 closest = a + edge_dir * proj;
                    float dist_sqr = LengthSqr(point - closest);
                    if (dist_sqr <= segment_radius_sqr) return true;
                }
                return false;
            };

            if (s.sample_count == 0) {
                Vec2 v1 = shape->anchors[s.anchor1].position;
                if (test_edge(v0, v1)) {
                    result->segment_index = s_idx;
                    result->path_index = s.path_left != U16_MAX ? s.path_left : s.path_right;
                    return true;
                }
            } else {
                for (u16 si = 0; si < s.sample_count; ++si) {
                    Vec2 v1 = s.samples[si];
                    if (test_edge(v0, v1)) {
                        result->segment_index = s_idx;
                        result->path_index = s.path_left != U16_MAX ? s.path_left : s.path_right;
                        return true;
                    }
                    v0 = v1;
                }

                // last segment to anchor1
                Vec2 v1 = shape->anchors[s.anchor1].position;
                if (test_edge(v0, v1)) {
                    result->segment_index = s_idx;
                    result->path_index = s.path_left != U16_MAX ? s.path_left : s.path_right;
                    return true;
                }
            }
        }

        // No anchor or segment hit — check filled paths last (anchor > segment > path)
        for (u16 p_idx = 0; p_idx < shape->path_count; ++p_idx) {
            Path& p = shape->paths[p_idx];
            if (!IsActive(&p) || p.segment_count < 1) continue;

            std::vector<Vec2> verts;
            verts.reserve(p.segment_count * (SHAPE_MAX_SEGMENT_SAMPLES + 2));

            for (u16 si = 0; si < p.segment_count; ++si) {
                u16 seg_idx = p.segments[si];
                if (seg_idx >= shape->segment_count) continue;
                Segment& s = shape->segments[seg_idx];
                if (!IsActive(&s)) continue;

                UpdateSegmentSamples(shape, seg_idx);

                Vec2 a0 = shape->anchors[s.anchor0].position;
                Vec2 a1 = shape->anchors[s.anchor1].position;

                verts.push_back(a0);
                for (u16 k = 0; k < s.sample_count; ++k) verts.push_back(s.samples[k]);
                verts.push_back(a1);
            }

            if (verts.size() < 3) continue;

            // Winding number algorithm (top-left fill rule) same as rasterizer
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

        return false;
    }

    void UpdateSegmentPaths(Shape* shape) {
        // Clear existing links
        for (u16 s_idx = 0; s_idx < shape->segment_count; ++s_idx) {
            Segment& s = shape->segments[s_idx];
            s.path_left = U16_MAX;
            s.path_right = U16_MAX;
        }

        // Populate left/right path indices for each segment
        for (u16 p_idx = 0; p_idx < shape->path_count; ++p_idx) {
            Path& p = shape->paths[p_idx];
            for (u16 i = 0; i < p.segment_count; ++i) {
                u16 seg_idx = p.segments[i];
                if (seg_idx >= shape->segment_count) continue;
                Segment& s = shape->segments[seg_idx];
                if (s.path_left == U16_MAX) s.path_left = p_idx;
                else if (s.path_right == U16_MAX) s.path_right = p_idx;
                // If more than two paths reference the same segment we ignore extras
            }
        }
    }

    void ClearSelection(Shape* shape) {
        for (u16 a_idx = 0; a_idx < SHAPE_MAX_ANCHORS; ++a_idx)
            ClearFlags(&shape->anchors[a_idx], ANCHOR_FLAG_SELECTED);            

        for (u16 s_idx = 0; s_idx < SHAPE_MAX_SEGMENTS; ++s_idx)
            ClearFlags(&shape->segments[s_idx], SEGMENT_FLAG_SELECTED);

        for (u16 p_idx = 0; p_idx < SHAPE_MAX_PATHS; ++p_idx)
            ClearFlags(&shape->paths[p_idx], PATH_FLAG_SELECTED);
    }
}