//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

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
