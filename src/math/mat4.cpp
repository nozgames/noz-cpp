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



