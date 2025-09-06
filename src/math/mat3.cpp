//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <cmath>

Mat3 TRS(const Vec2& translation, f32 rotation, const Vec2& scale)
{
    f32 c = std::cos(rotation);
    f32 s = std::sin(rotation);
    
    return Mat3{
        scale.x * c,     scale.x * s,     0,
        scale.y * -s,    scale.y * c,     0,
        translation.x,   translation.y,   1
    };
}

Mat3 Translate(const Vec2& translation)
{
    return Mat3{
        1, 0, 0,
        0, 1, 0,
        translation.x, translation.y, 1
    };
}

Mat3 Inverse(const Mat3& m)
{
    Mat3 result;
    const f32* mat = m.m;
    f32* inv = result.m;

    // Calculate determinant
    f32 det = mat[0] * (mat[4] * mat[8] - mat[7] * mat[5]) -
              mat[1] * (mat[3] * mat[8] - mat[5] * mat[6]) + 
              mat[2] * (mat[3] * mat[7] - mat[4] * mat[6]);

    if (det == 0.0f)
        return MAT3_IDENTITY; // Return identity if not invertible

    f32 inv_det = 1.0f / det;

    // Calculate inverse matrix elements
    inv[0] = (mat[4] * mat[8] - mat[7] * mat[5]) * inv_det;
    inv[1] = (mat[2] * mat[7] - mat[1] * mat[8]) * inv_det;
    inv[2] = (mat[1] * mat[5] - mat[2] * mat[4]) * inv_det;
    inv[3] = (mat[5] * mat[6] - mat[3] * mat[8]) * inv_det;
    inv[4] = (mat[0] * mat[8] - mat[2] * mat[6]) * inv_det;
    inv[5] = (mat[2] * mat[3] - mat[0] * mat[5]) * inv_det;
    inv[6] = (mat[3] * mat[7] - mat[4] * mat[6]) * inv_det;
    inv[7] = (mat[1] * mat[6] - mat[0] * mat[7]) * inv_det;
    inv[8] = (mat[0] * mat[4] - mat[1] * mat[3]) * inv_det;

    return result;
}

Vec2 Mat3::operator*(const Vec2& v) const
{
    return Vec2{
        m[0] * v.x + m[3] * v.y + m[6],
        m[1] * v.x + m[4] * v.y + m[7],
    };
}

Vec3 Mat3::operator*(const Vec3& v) const
{
    Vec3 result;
    result.x = m[0] * v.x + m[3] * v.y + m[6] * v.z;
    result.y = m[1] * v.x + m[4] * v.y + m[7] * v.z;
    result.z = m[2] * v.x + m[5] * v.y + m[8] * v.z;
    return result;
}
