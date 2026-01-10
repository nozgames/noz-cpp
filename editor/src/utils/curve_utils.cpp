//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    float FindBezierParameter(Vec2 p0, Vec2 control, Vec2 p2, Vec2 point) {
        float t = 0.5f;
        Vec2 edge = p2 - p0;
        float edge_len_sq = LengthSqr(edge);
        if (edge_len_sq > F32_EPSILON)
            t = Clamp(Dot(point - p0, edge) / edge_len_sq, 0.0f, 1.0f);

        // Newton-Raphson refinement
        for (int i = 0; i < 4; i++) {
            float mt = 1.0f - t;
            // B(t) = mt^2*p0 + 2*t*mt*control + t^2*p2
            Vec2 curve_pt = p0 * (mt * mt) + control * (2.0f * t * mt) + p2 * (t * t);
            // B'(t) = 2*(mt*(control-p0) + t*(p2-control))
            Vec2 derivative = (control - p0) * (2.0f * mt) + (p2 - control) * (2.0f * t);

            Vec2 diff = curve_pt - point;
            float deriv_len_sq = LengthSqr(derivative);
            if (deriv_len_sq < F32_EPSILON) break;

            float dt = Dot(diff, derivative) / deriv_len_sq;
            t = Clamp(t - dt, 0.0f, 1.0f);
        }

        return t;
    }

    Vec2 CalculateCircleOffset(Vec2 p0, Vec2 p1, const Vec2& centroid) {
        Vec2 midpoint = (p0 + p1) * 0.5f;

        // Tangent at each vertex is perpendicular to radius
        Vec2 r0 = p0 - centroid;
        Vec2 r1 = p1 - centroid;
        Vec2 t0 = Perpendicular(r0);
        Vec2 t1 = Perpendicular(r1);

        // Find intersection of tangent lines
        float denom = t0.x * t1.y - t0.y * t1.x;
        if (Abs(denom) < 0.0001f)
            return VEC2_ZERO;

        Vec2 dp = p1 - p0;
        float s = (dp.x * t1.y - dp.y * t1.x) / denom;
        Vec2 control_point = p0 + t0 * s;

        // Scale by Bezier factor for better circle approximation
        return (control_point - midpoint) * CIRCLE_BEZIER_FACTOR;
    }

    void SplitBezierCurve(Vec2 p0, Vec2 p2, Vec2 curve_offset, float curve_weight, float t,
                          Vec2* left_offset, float* left_weight, Vec2* right_offset, float* right_weight) {
        // Control point from curve_offset
        Vec2 p1 = (p0 + p2) * 0.5f + curve_offset;
        float w = curve_weight;
        float u = 1.0f - t;

        // De Casteljau subdivision for rational bezier
        // The control points split the same way, but weights need special handling
        Vec2 l1 = Mix(p0, p1, t);      // Left curve control point
        Vec2 r1 = Mix(p1, p2, t);      // Right curve control point
        Vec2 m = Mix(l1, r1, t);       // Split point (on curve at parameter t)

        // For rational quadratic, adjust the split point by weights
        // The split point in homogeneous coords factors through the weight
        float w_denom = u * u + 2.0f * u * t * w + t * t;
        if (w_denom > F32_EPSILON) {
            Vec2 p0_h = p0 * (u * u);
            Vec2 p1_h = p1 * (2.0f * u * t * w);
            Vec2 p2_h = p2 * (t * t);
            m = (p0_h + p1_h + p2_h) / w_denom;
        }

        // Weight subdivision for rational bezier:
        // w_left = sqrt(u + t*w), w_right = sqrt(t + u*w)
        // These preserve the rational property when splitting circular arcs
        *left_weight = sqrtf(u + t * w);
        *right_weight = sqrtf(t + u * w);

        // Left curve: [p0, l1, m]
        Vec2 left_mid = (p0 + m) * 0.5f;
        *left_offset = l1 - left_mid;

        // Right curve: [m, r1, p2]
        Vec2 right_mid = (m + p2) * 0.5f;
        *right_offset = r1 - right_mid;
    }

    float CalculateCircleWeight(Vec2 p0, Vec2 p1, Vec2 center) {
        // Weight = cos(half_angle) where angle is the arc span
        Vec2 r0 = Normalize(p0 - center);
        Vec2 r1 = Normalize(p1 - center);
        float cos_angle = Clamp(Dot(r0, r1), -1.0f, 1.0f);
        float half_angle = acosf(cos_angle) * 0.5f;
        return cosf(half_angle);
    }

    Vec2 EvalQuadraticBezier(Vec2 p0, Vec2 control, Vec2 p1, float t, float w) {
        float u = 1.0f - t;
        float u2 = u * u;
        float t2 = t * t;
        float wt = 2.0f * u * t * w;
        float denom = u2 + wt + t2;
        return (p0 * u2 + control * wt + p1 * t2) / denom;
    }

    bool LineCurveIntersect(Vec2 line_start, Vec2 line_end, Vec2 p0, Vec2 control, Vec2 p2, Vec2* out_point, float* out_t) {
        // Test line against subdivided curve segments (matches visual tessellation)
        constexpr int segments = 8;
        Vec2 prev = p0;

        for (int s = 1; s <= segments; s++) {
            float t = (float)s / (float)segments;
            float mt = 1.0f - t;
            Vec2 curr = p0 * (mt * mt) + control * (2.0f * t * mt) + p2 * (t * t);

            // Test for line-segment intersection
            Vec2 intersection;
            if (OverlapLine(line_start, line_end, prev, curr, &intersection)) {
                // Calculate t along this segment
                Vec2 seg_dir = curr - prev;
                float seg_len = Length(seg_dir);
                float seg_t = (seg_len > F32_EPSILON) ? Length(intersection - prev) / seg_len : 0.5f;

                // Convert to t along full curve
                float curve_t = ((float)(s - 1) + seg_t) / (float)segments;

                if (out_point) *out_point = intersection;  // Return segment intersection point
                if (out_t) *out_t = curve_t;
                return true;
            }

            prev = curr;
        }

        return false;
    }
}
