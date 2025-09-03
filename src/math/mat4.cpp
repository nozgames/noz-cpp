//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/noz_math.h>

Mat3::operator Mat4() const
{
    Mat4 result;
    result.m[0] = m[0];   result.m[1] = m[1];   result.m[2] = 0.0f;   result.m[3] = 0.0f;
    result.m[4] = m[3];   result.m[5] = m[4];   result.m[6] = 0.0f;   result.m[7] = 0.0f;
    result.m[8] = 0.0f;   result.m[9] = 0.0f;   result.m[10] = 1.0f;  result.m[11] = 0.0f;
    result.m[12] = m[6];  result.m[13] = m[7];  result.m[14] = 0.0f;  result.m[15] = 1.0f;
    return result;
}

Mat4 Ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    Mat4 result = MAT4_IDENTITY;
    
    result.m[0] = 2.0f / (right - left);
    result.m[5] = 2.0f / (top - bottom);
    result.m[10] = -2.0f / (far - near);
    
    result.m[12] = -(right + left) / (right - left);
    result.m[13] = -(top + bottom) / (top - bottom);
    result.m[14] = -(far + near) / (far - near);
    
    return result;
}

Mat4 Ortho(f32 top, f32 bottom, f32 near, f32 far)
{
    f32 height = Abs(top - bottom);
    f32 width = height * GetScreenAspectRatio();
    f32 left = -width * 0.5f;
    f32 right = width * 0.5f;
    return Ortho(left, right, bottom, top, near, far);
}

Mat4 Mat4::operator*(const Mat4& o) const
{
    // multiply
    Mat4 result;
    for (int row = 0; row < 4; row++)
    {
        for (int col = 0; col < 4; col++)
        {
            result.m[col + row * 4] =
                m[0 + row * 4] * o.m[col + 0 * 4] +
                m[1 + row * 4] * o.m[col + 1 * 4] +
                m[2 + row * 4] * o.m[col + 2 * 4] +
                m[3 + row * 4] * o.m[col + 3 * 4];
        }
    }

    return result;
}

Vec3 Mat4::operator*(const Vec3& v) const
{
    return Vec3{
        m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12],
        m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13],
        m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14]
    };
}

Vec4 Mat4::operator*(const Vec4& v) const
{
    return Vec4{
        m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w,
        m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
        m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w,
        m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w
    };
}

Mat4 Inverse(const Mat4& m)
{
    Mat4 result;
    const f32* mat = m.m;
    f32* inv = result.m;

    inv[0] = mat[5] * mat[10] * mat[15] - 
             mat[5] * mat[11] * mat[14] - 
             mat[9] * mat[6] * mat[15] + 
             mat[9] * mat[7] * mat[14] +
             mat[13] * mat[6] * mat[11] - 
             mat[13] * mat[7] * mat[10];

    inv[4] = -mat[4] * mat[10] * mat[15] + 
              mat[4] * mat[11] * mat[14] + 
              mat[8] * mat[6] * mat[15] - 
              mat[8] * mat[7] * mat[14] - 
              mat[12] * mat[6] * mat[11] + 
              mat[12] * mat[7] * mat[10];

    inv[8] = mat[4] * mat[9] * mat[15] - 
             mat[4] * mat[11] * mat[13] - 
             mat[8] * mat[5] * mat[15] + 
             mat[8] * mat[7] * mat[13] + 
             mat[12] * mat[5] * mat[11] - 
             mat[12] * mat[7] * mat[9];

    inv[12] = -mat[4] * mat[9] * mat[14] + 
               mat[4] * mat[10] * mat[13] +
               mat[8] * mat[5] * mat[14] - 
               mat[8] * mat[6] * mat[13] - 
               mat[12] * mat[5] * mat[10] + 
               mat[12] * mat[6] * mat[9];

    inv[1] = -mat[1] * mat[10] * mat[15] + 
              mat[1] * mat[11] * mat[14] + 
              mat[9] * mat[2] * mat[15] - 
              mat[9] * mat[3] * mat[14] - 
              mat[13] * mat[2] * mat[11] + 
              mat[13] * mat[3] * mat[10];

    inv[5] = mat[0] * mat[10] * mat[15] - 
             mat[0] * mat[11] * mat[14] - 
             mat[8] * mat[2] * mat[15] + 
             mat[8] * mat[3] * mat[14] + 
             mat[12] * mat[2] * mat[11] - 
             mat[12] * mat[3] * mat[10];

    inv[9] = -mat[0] * mat[9] * mat[15] + 
              mat[0] * mat[11] * mat[13] + 
              mat[8] * mat[1] * mat[15] - 
              mat[8] * mat[3] * mat[13] - 
              mat[12] * mat[1] * mat[11] + 
              mat[12] * mat[3] * mat[9];

    inv[13] = mat[0] * mat[9] * mat[14] - 
              mat[0] * mat[10] * mat[13] - 
              mat[8] * mat[1] * mat[14] + 
              mat[8] * mat[2] * mat[13] + 
              mat[12] * mat[1] * mat[10] - 
              mat[12] * mat[2] * mat[9];

    inv[2] = mat[1] * mat[6] * mat[15] - 
             mat[1] * mat[7] * mat[14] - 
             mat[5] * mat[2] * mat[15] + 
             mat[5] * mat[3] * mat[14] + 
             mat[13] * mat[2] * mat[7] - 
             mat[13] * mat[3] * mat[6];

    inv[6] = -mat[0] * mat[6] * mat[15] + 
              mat[0] * mat[7] * mat[14] + 
              mat[4] * mat[2] * mat[15] - 
              mat[4] * mat[3] * mat[14] - 
              mat[12] * mat[2] * mat[7] + 
              mat[12] * mat[3] * mat[6];

    inv[10] = mat[0] * mat[5] * mat[15] - 
              mat[0] * mat[7] * mat[13] - 
              mat[4] * mat[1] * mat[15] + 
              mat[4] * mat[3] * mat[13] + 
              mat[12] * mat[1] * mat[7] - 
              mat[12] * mat[3] * mat[5];

    inv[14] = -mat[0] * mat[5] * mat[14] + 
               mat[0] * mat[6] * mat[13] + 
               mat[4] * mat[1] * mat[14] - 
               mat[4] * mat[2] * mat[13] - 
               mat[12] * mat[1] * mat[6] + 
               mat[12] * mat[2] * mat[5];

    inv[3] = -mat[1] * mat[6] * mat[11] + 
              mat[1] * mat[7] * mat[10] + 
              mat[5] * mat[2] * mat[11] - 
              mat[5] * mat[3] * mat[10] - 
              mat[9] * mat[2] * mat[7] + 
              mat[9] * mat[3] * mat[6];

    inv[7] = mat[0] * mat[6] * mat[11] - 
             mat[0] * mat[7] * mat[10] - 
             mat[4] * mat[2] * mat[11] + 
             mat[4] * mat[3] * mat[10] + 
             mat[8] * mat[2] * mat[7] - 
             mat[8] * mat[3] * mat[6];

    inv[11] = -mat[0] * mat[5] * mat[11] + 
               mat[0] * mat[7] * mat[9] + 
               mat[4] * mat[1] * mat[11] - 
               mat[4] * mat[3] * mat[9] - 
               mat[8] * mat[1] * mat[7] + 
               mat[8] * mat[3] * mat[5];

    inv[15] = mat[0] * mat[5] * mat[10] - 
              mat[0] * mat[6] * mat[9] - 
              mat[4] * mat[1] * mat[10] + 
              mat[4] * mat[2] * mat[9] + 
              mat[8] * mat[1] * mat[6] - 
              mat[8] * mat[2] * mat[5];

    f32 det = mat[0] * inv[0] + mat[1] * inv[4] + mat[2] * inv[8] + mat[3] * inv[12];

    if (det == 0)
        return MAT4_IDENTITY;

    det = 1.0f / det;

    for (int i = 0; i < 16; i++)
        inv[i] = inv[i] * det;

    return result;
}

