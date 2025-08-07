/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz
{
    class StreamReader;
    class StreamWriter;
}

namespace noz::ui
{
	enum class StyleKeyword
	{
		Inherit,
		Overwrite
	};

    struct StyleParameter
    {
        StyleKeyword keyword;

        StyleParameter()
        {
            keyword = StyleKeyword::Inherit;
        }

        StyleParameter(StyleKeyword kw) : keyword(kw) {}

        virtual ~StyleParameter() = default;

        // Pure virtual serialization methods
        virtual void serialize(StreamWriter& writer) const = 0;
        virtual void deserialize(StreamReader& reader) = 0;

        // Helper methods
        bool isInherited() const { return keyword == StyleKeyword::Inherit; }
        bool isOverwrite() const { return keyword == StyleKeyword::Overwrite; }

        static StyleParameter* inherit() 
        { 
            // Note: This would need to be implemented by derived classes
            // since we can't instantiate abstract base class
            return nullptr;
        }

    protected:
        // Helper method for derived classes to set overwrite mode
        void setOverwrite() { keyword = StyleKeyword::Overwrite; }
    };
}