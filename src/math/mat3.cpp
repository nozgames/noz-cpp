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
