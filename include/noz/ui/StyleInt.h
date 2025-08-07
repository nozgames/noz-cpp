/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/ui/StyleParameter.h>

namespace noz::ui
{
    struct StyleInt : public StyleParameter
    {
        int value;
        
        StyleInt() : StyleParameter()
        {
            value = 0;
        }
        
        StyleInt(int v) : StyleParameter(StyleKeyword::Overwrite)
        {
            value = v;
        }
        
        static StyleInt inherit() 
        { 
            StyleInt i; 
            i.keyword = StyleKeyword::Inherit; 
            return i; 
        }
        
        StyleInt& operator=(int v)
        {
            setOverwrite();
            value = v;
            return *this;
        }
        
        operator int() const { return value; }
        
        // Virtual serialization methods
        void serialize(StreamWriter& writer) const override;
        void deserialize(StreamReader& reader) override;
    };
}