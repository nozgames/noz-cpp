/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/ui/StyleParameter.h>
#include <cstdint>

namespace noz::ui
{
    enum class FlexDirection
    {
        Row,
        Column,
        RowReverse,
        ColumnReverse
    };
    template<typename T>
    struct StyleEnum : public StyleParameter
    {
        T value;
        
        StyleEnum() : StyleParameter()
        {
            value = T{};
        }
        
        StyleEnum(T v) : StyleParameter(StyleKeyword::Overwrite)
        {
            value = v;
        }
        
        static StyleEnum<T> inherit() 
        { 
            StyleEnum<T> e; 
            e.keyword = StyleKeyword::Inherit; 
            return e; 
        }
        
        // Parse StyleEnum from string (for metafile usage)
        // This is a generic template - specific implementations should be specialized
        static StyleEnum<T> parse(const std::string& value, const StyleEnum<T>& defaultValue = StyleEnum<T>())
        {
            // Default implementation just returns the default value
            // Specific enum types should specialize this method
            return defaultValue;
        }
        
        StyleEnum& operator=(T v)
        {
            setOverwrite();
            value = v;
            return *this;
        }
        
        operator T() const { return value; }
        
        // Virtual serialization methods
        void serialize(StreamWriter& writer) const override
        {
            writer.write(static_cast<uint8_t>(keyword));
            writer.write(static_cast<uint8_t>(value));
        }
        
        void deserialize(StreamReader& reader) override
        {
            keyword = static_cast<StyleKeyword>(reader.read<uint8_t>());
            value = static_cast<T>(reader.read<uint8_t>());
        }
    };

    // Template specialization for FlexDirection parsing
    template<>
    inline StyleEnum<FlexDirection> StyleEnum<FlexDirection>::parse(const std::string& propertyValue, const StyleEnum<FlexDirection>& defaultValue)
    {
        try
        {
            if (propertyValue == "row")
            {
                return StyleEnum<FlexDirection>(FlexDirection::Row);
            }
            else if (propertyValue == "column")
            {
                return StyleEnum<FlexDirection>(FlexDirection::Column);
            }
            else if (propertyValue == "row-reverse")
            {
                return StyleEnum<FlexDirection>(FlexDirection::RowReverse);
            }
            else if (propertyValue == "column-reverse")
            {
                return StyleEnum<FlexDirection>(FlexDirection::ColumnReverse);
            }
            else
            {
                return StyleEnum<FlexDirection>(FlexDirection::Row);
            }
        }
        catch (const std::exception&)
        {
            // Return default value on parse error
            return defaultValue;
        }
    }
}