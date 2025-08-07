#pragma once

#include "../ttf/TrueTypeFont.h"

namespace noz::import::msdf
{
	void renderGlyph(
		const noz::import::ttf::TrueTypeFont::Glyph* glyph,
		std::vector<uint8_t>& output,
		int outputStride,
		const glm::ivec2& outputPosition,
		const glm::ivec2& outputSize,
		double range,
		const glm::dvec2& scale,
		const glm::dvec2& translate
	);
}
