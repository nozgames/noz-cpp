//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include <cmath>

Mat3 TRS(const Vec2& translation, f32 rotation, const Vec2& scale)
{
    rotation = Radians(rotation);
    f32 c = std::cos(rotation);
    f32 s = std::sin(rotation);
    return TRS(translation, {c, s}, scale);
}

Mat3 TRS(const Vec2& translation, const Vec2& direction, const Vec2& scale)
{
    Vec2 perp = Perpendicular(direction);

    return Mat3{
        scale.x * direction.x,  scale.x * direction.y,  0,
        scale.y * perp.x,       scale.y * perp.y,       0,
        translation.x,          translation.y,          1
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

Mat3 Rotate(f32 rotation) {
    rotation = Radians(rotation);
    f32 c = std::cos(rotation);
    f32 s = std::sin(rotation);
    return Mat3{
        c, s, 0,
       -s, c, 0,
        0, 0, 1
    };
}

Mat3 Scale(f32 scale)
{
    return Mat3{
        scale, 0,     0,
        0,     scale, 0,
        0,     0,     1
    };
}

Mat3 Scale(const Vec2& scale)
{
    return Mat3{
        scale.x, 0,       0,
        0,       scale.y, 0,
        0,       0,       1
    };
}

extern Mat3 Rotate(const Vec2& direction)
{
    Vec2 dir = Normalize(direction);
    Vec2 perp = Perpendicular(dir);
    return Mat3{
        dir.x,  dir.y, 0,
        perp.x, perp.y, 0,
        0,      0,     1
    };
}

Mat3 Inverse(const Mat3& m)
{
    Mat3 result;
    const f32* mat = m.m;
    f32* inv = result.m;

    // Calculate determinant
    f32 det = mat[0] * (mat[4] * mat[8] - mat[5] * mat[7]) -
              mat[1] * (mat[3] * mat[8] - mat[5] * mat[6]) +
              mat[2] * (mat[3] * mat[7] - mat[4] * mat[6]);

    if (det == 0.0f)
        return MAT3_IDENTITY;

    f32 inv_det = 1.0f / det;

    // Calculate inverse (transposed cofactor matrix / determinant)
    inv[0] = (mat[4] * mat[8] - mat[5] * mat[7]) * inv_det;
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

Mat3 Mat3::operator*(const Mat3& o) const
{
    Mat3 result;
    result.m[0] = this->m[0] * o.m[0] + this->m[3] * o.m[1] + this->m[6] * o.m[2];
    result.m[1] = this->m[1] * o.m[0] + this->m[4] * o.m[1] + this->m[7] * o.m[2];
    result.m[2] = this->m[2] * o.m[0] + this->m[5] * o.m[1] + this->m[8] * o.m[2];
    result.m[3] = this->m[0] * o.m[3] + this->m[3] * o.m[4] + this->m[6] * o.m[5];
    result.m[4] = this->m[1] * o.m[3] + this->m[4] * o.m[4] + this->m[7] * o.m[5];
    result.m[5] = this->m[2] * o.m[3] + this->m[5] * o.m[4] + this->m[8] * o.m[5];
    result.m[6] = this->m[0] * o.m[6] + this->m[3] * o.m[7] + this->m[6] * o.m[8];
    result.m[7] = this->m[1] * o.m[6] + this->m[4] * o.m[7] + this->m[7] * o.m[8];
    result.m[8] = this->m[2] * o.m[6] + this->m[5] * o.m[7] + this->m[8] * o.m[8];
    return result;
}

Vec2 TransformPoint(const Mat3& m, const Vec2& point)
{
    return Vec2{
        m.m[0] * point.x + m.m[3] * point.y + m.m[6],
        m.m[1] * point.x + m.m[4] * point.y + m.m[7],
    };
}

Vec2 TransformVector(const Mat3& m, const Vec2& vector)
{
    return Vec2{
        m.m[0] * vector.x + m.m[3] * vector.y,
        m.m[1] * vector.x + m.m[4] * vector.y,
    };
}

Vec2 GetForward(const Mat3& m)
{
    return Vec2{ m.m[0], m.m[1] };
}

Vec2 GetRight(const Mat3& m)
{
    return Vec2{ m.m[3], m.m[4] };
}

f32 GetRotation(const Mat3& m) {
    return Degrees(Atan2(m.m[1], m.m[0]));
}

Mat3 Transpose(const Mat3& m) {
    return Mat3{
        m.m[0], m.m[3], m.m[6],
        m.m[1], m.m[4], m.m[7],
        m.m[2], m.m[5], m.m[8]
    };
}
