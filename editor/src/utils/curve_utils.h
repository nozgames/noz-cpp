//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

// Scale factor for quadratic Bezier circle approximation: 2*(sqrt(2)-1)
// This makes the curve pass through the actual circle at the midpoint
constexpr float CIRCLE_BEZIER_FACTOR = 0.82842712f;

// Find the Bezier parameter t for a point on/near a quadratic Bezier curve
// Uses Newton-Raphson iteration to find closest point on curve
extern float FindBezierParameter(Vec2 p0, Vec2 control, Vec2 p2, Vec2 point);

// Calculate the curve offset that would make the edge a circular arc around centroid
extern Vec2 CalculateCircleOffset(Vec2 p0, Vec2 p1, const Vec2& centroid);

// Find intersection between a line segment and a Bezier curve
// Returns true if intersection found, sets out_point and out_t (parameter on curve)
extern bool LineCurveIntersect(Vec2 line_start, Vec2 line_end, Vec2 p0, Vec2 control, Vec2 p2, Vec2* out_point, float* out_t);

// Split a quadratic Bezier curve at parameter t using de Casteljau's algorithm
// Returns the curve_offset for the left [p0, split_point] and right [split_point, p2] sub-curves
extern void SplitBezierCurve(Vec2 p0, Vec2 p2, Vec2 curve_offset, float t, Vec2* left_offset, Vec2* right_offset);
