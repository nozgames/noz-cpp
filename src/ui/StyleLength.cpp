/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/ui/StyleLength.h>

namespace noz::ui
{
	const StyleLength StyleLength::Auto(StyleLength::Unit::Auto, 0.0f);

	float StyleLength::evaluate(float parentValue) const
	{
		if (isAuto())
			return parentValue;

		switch (unit)
		{
		case StyleLength::Unit::Fixed:
			return value;

		case StyleLength::Unit::Percent:
			return parentValue * value;

		default:
			return 0.0f;
		}
	}

	StyleLength StyleLength::parse(const std::string& propertyValue, const StyleLength& defaultValue)
	{
		try
		{
			if (propertyValue == "auto")
				return StyleLength::Auto;
			else if (!propertyValue.empty() && propertyValue.back() == '%')
				return StyleLength::percent(std::stof(propertyValue.substr(0, propertyValue.length() - 1)) / 100.0f);
			else
				return StyleLength::fixed(std::stof(propertyValue));
		}
		catch (const std::exception&)
		{
			return defaultValue;
		}
	}

	void StyleLength::serialize(StreamWriter& writer) const
	{
		writer.write(static_cast<uint8_t>(keyword));
		writer.write(static_cast<uint8_t>(unit));
		writer.write(value);
	}

	void StyleLength::deserialize(StreamReader& reader)
	{
		keyword = static_cast<StyleKeyword>(reader.read<uint8_t>());
		unit = static_cast<StyleLength::Unit>(reader.read<uint8_t>());
		value = reader.read<float>();
	}
}
