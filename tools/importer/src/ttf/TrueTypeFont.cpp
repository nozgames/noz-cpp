/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "TrueTypeFont.h"
#include "TrueTypeFontReader.h"

namespace noz::import::ttf
{
	TrueTypeFont* TrueTypeFont::load(const std::string& path, int requestedSize, const std::string& filter)
	{
		std::vector<uint8_t> data;
		StreamReader reader(data);
		if (!reader.loadFromFile(path)) 
			throw std::runtime_error("Failed to load TrueType font from file: " + path);

		return load(&reader, requestedSize, filter);
	}

	TrueTypeFont* TrueTypeFont::load(noz::StreamReader* stream, int requestedSize, const std::string& filter)
	{
		TrueTypeFontReader reader(stream, requestedSize, filter);
		return reader.read();
	}
}
