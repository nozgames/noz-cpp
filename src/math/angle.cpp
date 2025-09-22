//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

float SignedAngleDelta(const Vec2& a, const Vec2&b)
{
    float angle_a = atan2f(a.y, a.x);
    float angle_b = atan2f(b.y, b.x);
    float delta = angle_b - angle_a;

    if (delta > noz::PI)
        delta -= noz::TWO_PI;
    else if (delta < -noz::PI)
        delta += noz::TWO_PI;

    return Degrees(delta);
}

float NormalizeAngle(float angle)
{
    while (angle < 0) angle += noz::TWO_PI;
    while (angle >= noz::TWO_PI) angle -= noz::TWO_PI;
    return angle;
}
