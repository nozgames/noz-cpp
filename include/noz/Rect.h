#pragma once

namespace noz
{
    struct Rect
    {
        float x;
        float y;
        float width;
        float height;

        Rect();
        Rect(float x, float y, float width, float height);

        float left() const;
        float right() const;
        float top() const;
        float bottom() const;

        void setLeft(float left);
        void setRight(float right);
        void setTop(float top);
        void setBottom(float bottom);

        bool contains(float px, float py) const;
        bool intersects(const Rect& other) const;
        Rect intersection(const Rect& other) const;
    };
} 