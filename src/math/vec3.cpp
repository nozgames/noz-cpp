//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

float Length(const Vec3& v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vec3 Normalize(const Vec3& v)
{
    f32 len = Length(v);
    if (len <= 0.0f)
        return {0,0,0};

    len = 1.0f / len;
    return { v.x * len, v.y * len, v.z * len };
}

Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}
