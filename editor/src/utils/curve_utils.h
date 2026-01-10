//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::editor {

    // Scale factor for quadratic Bezier circle approximation
    // 1.0 = control point at tangent intersection (slight outward bulge ~6%)
    // Lower values flatten the curve
    constexpr float CIRCLE_BEZIER_FACTOR = 1.0f;

    // Find the Bezier parameter t for a point on/near a quadratic Bezier curve
    // Uses Newton-Raphson iteration to find closest point on curve
    extern float FindBezierParameter(Vec2 p0, Vec2 control, Vec2 p2, Vec2 point);

    // Calculate the curve offset that would make the edge a circular arc around centroid
    extern Vec2 CalculateCircleOffset(Vec2 p0, Vec2 p1, const Vec2& centroid);

    // Find intersection between a line segment and a Bezier curve
    // Returns true if intersection found, sets out_point and out_t (parameter on curve)
    extern bool LineCurveIntersect(Vec2 line_start, Vec2 line_end, Vec2 p0, Vec2 control, Vec2 p2, Vec2* out_point, float* out_t);

    // Split a rational quadratic Bezier curve at parameter t using de Casteljau's algorithm
    // Returns the curve_offset and weight for the left [p0, split_point] and right [split_point, p2] sub-curves
    extern void SplitBezierCurve(Vec2 p0, Vec2 p2, Vec2 curve_offset, float curve_weight, float t,
                                 Vec2* left_offset, float* left_weight, Vec2* right_offset, float* right_weight);

    // Calculate the weight for a perfect circular arc between p0 and p1 around center
    extern float CalculateCircleWeight(Vec2 p0, Vec2 p1, Vec2 center);

    // Evaluate a rational quadratic Bezier curve at parameter t
    // w = 1.0 gives standard bezier, w = cos(half_angle) gives circular arc
    extern Vec2 EvalQuadraticBezier(Vec2 p0, Vec2 control, Vec2 p1, float t, float w);
}
