//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

f32 Length(const Vec2& v)
{
    return sqrtf(v.x * v.x + v.y * v.y);
}

Vec2 Reflect(const Vec2& v, const Vec2& normal)
{
    f32 dot = v.x * normal.x + v.y * normal.y;
    return Vec2{ v.x - 2.0f * dot * normal.x, v.y - 2.0f * dot * normal.y };
}

Vec2 Normalize(const Vec2& v)
{
    f32 length = sqrt(v.x * v.x + v.y * v.y);
    if (length > 0.0f) {
        return Vec2{ v.x / length, v.y / length };
    }
    return Vec2{ 0.0f, 0.0f };
}

Vec2 Rotate(const Vec2& v, f32 degrees)
{
    f32 radians = degrees * noz::DEG_TO_RAD;
    f32 cos_angle = cosf(radians);
    f32 sin_angle = sinf(radians);
    
    return Vec2{
        v.x * cos_angle - v.y * sin_angle,
        v.x * sin_angle + v.y * cos_angle
    };
}

f64 Length(const Vec2Double& v)
{
    return sqrt(v.x * v.x + v.y * v.y);
}

Vec2Double Normalize(const Vec2Double& v)
{
    f64 length = sqrt(v.x * v.x + v.y * v.y);
    if (length <= 0.0f)
        return { 0.0, 0.0 };

    length = 1.0 / length;
    return { v.x * length, v.y * length };
}
