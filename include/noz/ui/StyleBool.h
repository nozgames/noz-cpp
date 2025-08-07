/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/ui/StyleParameter.h>

namespace noz::ui
{
    struct StyleBool : public StyleParameter
    {
        bool value;
        
        StyleBool() : StyleParameter()
        {
            value = false;
        }
        
        StyleBool(bool v) : StyleParameter(StyleKeyword::Overwrite)
        {
            value = v;
        }
        
        static StyleBool inherit() 
        { 
            StyleBool b; 
            b.keyword = StyleKeyword::Inherit; 
            return b; 
        }
        
        StyleBool& operator=(bool v)
        {
            setOverwrite();
            value = v;
            return *this;
        }
        
        operator bool() const { return value; }
        
        // Virtual serialization methods
        void serialize(StreamWriter& writer) const override;
        void deserialize(StreamReader& reader) override;
    };
}