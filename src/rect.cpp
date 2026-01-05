//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz {
    Rect Intersection(const Rect& rect, const Rect& other) {
        f32 left = Max(rect.x, other.x);
        f32 top = Max(rect.y, other.y);
        f32 right = Min(rect.x + rect.width, other.x + other.width);
        f32 bottom = Min(rect.y + rect.height, other.y + other.height);

        if (left < right && top < bottom)
            return {left, top, right - left, bottom - top };

        return {};
    }


    RectInt Union(const RectInt& r1, const RectInt& r2) {
        int x_max = Max(r1.x + r1.w, r2.x + r2.w);
        int y_max = Max(r1.y + r1.h, r2.y + r2.h);
        int x = Min(r1.x, r2.x);
        int y = Min(r1.y, r2.y);
        int w = x_max - x;
        int h = y_max - y;
        return RectInt {x,y,w,h};
    }
}

