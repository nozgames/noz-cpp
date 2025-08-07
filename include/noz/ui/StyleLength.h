/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/ui/StyleParameter.h>

namespace noz::ui
{
    struct StyleLength : public StyleParameter
    {
        enum class Unit
        {
            Fixed,
            Percent,
            Auto
        };

        static const StyleLength Auto;

        Unit unit;
        float value;

        StyleLength() : StyleParameter()
        {
            unit = Unit::Fixed;
            value = 0.0f;
        }

        StyleLength(Unit unit, float value) : StyleParameter(StyleKeyword::Overwrite)
        {
            this->unit = unit;
            this->value = value;
        }

        bool isAuto() const { return unit == Unit::Auto; }
        bool isFixed() const { return unit == Unit::Fixed; }

        static StyleLength percent(float value);
        static StyleLength fixed(float value);
        
        // Parse StyleLength from string (for metafile usage)
        static StyleLength parse(const std::string& value, const StyleLength& defaultValue = StyleLength::Auto);

        StyleLength& operator= (float fixed) 
        {
            *this = StyleLength::fixed(fixed);
            return *this;
        }

        float evaluate(float parentValue) const;
        
        // Virtual serialization methods
        void serialize(StreamWriter& writer) const override;
        void deserialize(StreamReader& reader) override;
    };

	inline StyleLength StyleLength::percent(float value)
	{
		return StyleLength(StyleLength::Unit::Percent, value);
	}

	inline StyleLength StyleLength::fixed(float value)
	{
		return StyleLength(StyleLength::Unit::Fixed, value);
	}
}