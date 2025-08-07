/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
    Rect::Rect() : x(0.0f), y(0.0f), width(0.0f), height(0.0f) {}

    Rect::Rect(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}

    float Rect::left() const { return x; }

    float Rect::right() const { return x + width; }

    float Rect::top() const { return y; }

    float Rect::bottom() const { return y + height; }

    void Rect::setLeft(float left) { width += x - left; x = left; }

    void Rect::setRight(float right) { width = right - x; }

    void Rect::setTop(float top) { height += y - top; y = top; }

    void Rect::setBottom(float bottom) { height = bottom - y; }

    bool Rect::contains(float px, float py) const
    {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }

    bool Rect::intersects(const Rect& other) const
    {
        return !(other.x > x + width || other.x + other.width < x ||
                 other.y > y + height || other.y + other.height < y);
    }

    Rect Rect::intersection(const Rect& other) const
    {
        float left = std::max(x, other.x);
        float top = std::max(y, other.y);
        float right = std::min(x + width, other.x + other.width);
        float bottom = std::min(y + height, other.y + other.height);

        if (left < right && top < bottom)
            return Rect(left, top, right - left, bottom - top);
        else
            return Rect();
    }
} 