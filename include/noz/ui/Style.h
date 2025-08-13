/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/ui/StyleParameter.h>
#include <noz/ui/StyleLength.h>
#include <noz/ui/StyleColor.h>
#include <noz/ui/StyleFloat.h>
#include <noz/ui/StyleInt.h>
#include <noz/ui/StyleBool.h>
#include <noz/ui/StyleEnum.h>

namespace noz::ui
{
    enum class TextAlignment
    {
        TopLeft,
        TopMiddle,
        MiddleLeft,
        BottomLeft,
        Middle,
        TopRight,
        MiddleRight,
        BottomRight
    };

    enum class ScaleMode
    {
        ScaleToFit,
        StretchToFill,
        ScaleAndCrop
    };

    struct Style
    {
    private:

        static Style s_default;

    public:

		StyleEnum<FlexDirection> flexDirection;
        StyleLength width;
        StyleLength height;
        StyleColor backgroundColor;
        StyleColor color;
        StyleFloat fontSize;
        StyleFloat borderRadius;
        StyleFloat borderWidth;
        StyleColor borderColor;
        StyleInt font;
        StyleColor imageTint;
        StyleEnum<ScaleMode> imageScaleMode;
        StyleBool imageFlip;
        StyleEnum<TextAlignment> textAlignment;
        StyleFloat textOutlineWidth;
        StyleColor textOutlineColor;
        StyleLength marginTop;
        StyleLength marginLeft;
        StyleLength marginBottom;
        StyleLength marginRight;
        StyleLength paddingTop;
        StyleLength paddingLeft;
        StyleLength paddingBottom;
        StyleLength paddingRight;

        static Style default() { return s_default; }

        void apply(const Style& style);
        void apply(const Style* style);
        
        // Serialization
        void serialize(StreamWriter& writer) const;
        void deserialize(StreamReader& reader);
    };

    // Style extension methods for fluent API
    inline Style width(Style s, float value)
    {
        s.width = StyleLength::fixed(value);
        return s;
    }
    
    inline Style width(Style s, const StyleLength& value)
    {
        s.width = value;
        return s;
    }
    
    inline Style height(Style s, float value)
    {
        s.height = StyleLength::fixed(value);
        return s;
    }
    
    inline Style height(Style s, const StyleLength& height)
    {
        s.height = height;
        return s;
    }
    
    inline Style marginTop(Style s, const StyleLength& value)
    {
        s.marginTop = value;
        return s;
    }

    inline Style marginLeft(Style s, const StyleLength& value)
    {
        s.marginLeft = value;
        return s;
    }

    inline Style marginBottom(Style s, const StyleLength& value)
    {
        s.marginBottom = value;
        return s;
    }

    inline Style marginRight(Style s, const StyleLength& value)
    {
        s.marginRight = value;
        return s;
    }

    inline Style color(Style s, const Color& value)
    {
        s.color = StyleColor(value);
        return s;
    }
    
    inline Style backgroundColor(Style s, const Color& value)
    {
        s.backgroundColor = StyleColor(value);
        return s;
    }
    
    inline Style imageTint(Style s, const Color& value)
    {
        s.imageTint = StyleColor(value);
        return s;
    }
    
    inline Style margin(Style s, int size)
    {
        return marginTop(marginLeft(marginBottom(marginRight(s, StyleLength::fixed(static_cast<float>(size))), StyleLength::fixed(static_cast<float>(size))), StyleLength::fixed(static_cast<float>(size))), StyleLength::fixed(static_cast<float>(size)));
    }

    inline Style margin(Style s, const StyleLength& length)
    {
        return marginTop(marginLeft(marginBottom(marginRight(s, length), length), length), length);
    }

    inline Style paddingTop(Style s, const StyleLength& value)
    {
        s.paddingTop = value;
        return s;
    }

    inline Style paddingLeft(Style s, const StyleLength& value)
    {
        s.paddingLeft = value;
        return s;
    }

    inline Style paddingBottom(Style s, const StyleLength& value)
    {
        s.paddingBottom = value;
        return s;
    }

    inline Style paddingRight(Style s, const StyleLength& value)
    {
        s.paddingRight = value;
        return s;
    }

    inline Style padding(Style s, int size)
    {
        return paddingTop(paddingLeft(paddingBottom(paddingRight(s, StyleLength::fixed(static_cast<float>(size))), StyleLength::fixed(static_cast<float>(size))), StyleLength::fixed(static_cast<float>(size))), StyleLength::fixed(static_cast<float>(size)));
    }

    inline Style padding(Style s, const StyleLength& length)
    {
        return paddingTop(paddingLeft(paddingBottom(paddingRight(s, length), length), length), length);
    }
} 