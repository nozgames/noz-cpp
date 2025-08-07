/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/ui/StyleParameter.h>

namespace noz::ui
{
    struct StyleColor : public StyleParameter
    {
        Color value;
        
        StyleColor() : StyleParameter()
        {
            value = Color::Transparent;
        }
        
        StyleColor(const Color& color) : StyleParameter(StyleKeyword::Overwrite)
        {
            value = color;
        }
        
        static StyleColor inherit() 
        { 
            StyleColor c; 
            c.keyword = StyleKeyword::Inherit; 
            return c; 
        }
        
        StyleColor& operator=(const Color& color)
        {
            setOverwrite();
            value = color;
            return *this;
        }
        
        operator Color() const { return value; }
        
        // Virtual serialization methods
        void serialize(StreamWriter& writer) const override;
        void deserialize(StreamReader& reader) override;
    };
}