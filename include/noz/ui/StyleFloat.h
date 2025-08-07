/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/ui/StyleParameter.h>

namespace noz::ui
{
    struct StyleFloat : public StyleParameter
    {
        float value;
        
        StyleFloat() : StyleParameter()
        {
            value = 0.0f;
        }
        
        StyleFloat(float v) : StyleParameter(StyleKeyword::Overwrite)
        {
            value = v;
        }
        
        static StyleFloat inherit() 
        { 
            StyleFloat f; 
            f.keyword = StyleKeyword::Inherit; 
            return f; 
        }
        
        StyleFloat& operator=(float v)
        {
            setOverwrite();
            value = v;
            return *this;
        }
        
        operator float() const { return value; }
        
        // Virtual serialization methods
        void serialize(StreamWriter& writer) const override;
        void deserialize(StreamReader& reader) override;
    };
}