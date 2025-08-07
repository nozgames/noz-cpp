/*
  NoZ Game Engine

  Copyright(c) 2019 NoZ Games, LLC

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files(the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions :

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "msdf.h"
#include "Shape.h"

namespace noz::import::msdf
{
	void generateSDF(
		std::vector<uint8_t>& output,
		int outputStride,
		const glm::ivec2& outputPosition,
		const glm::ivec2& outputSize,
		const Shape& shape,
		double range,
		const glm::dvec2& scale,
		const glm::dvec2& translate)
	{
		int contourCount = (int)shape.contours.size();
		int w = outputSize.x;
		int h = outputSize.y;

		std::vector<int> windings;
		windings.resize(contourCount);
		for (size_t i = 0; i < shape.contours.size(); i++)
			windings[i] = shape.contours[i]->winding();

		std::vector<double> contourSD;
		contourSD.resize(contourCount);

		for (int y = 0; y < h; ++y)
		{
			int row = shape.inverseYAxis ? h - y - 1 : y;
			for (int x = 0; x < w; ++x)
			{
				auto dummy = 0.0;
				auto p = glm::dvec2(x + .5, y + .5) / scale - translate;
				auto negDist = -SignedDistance::Infinite.distance;
				auto posDist = SignedDistance::Infinite.distance;
				int winding = 0;

				for (size_t i = 0; i < shape.contours.size(); i++)
				{
					auto minDistance = SignedDistance::Infinite;
					for(auto edge : shape.contours[i]->edges)
					{
						auto distance = edge->distance(p, dummy);
						if (distance < minDistance)
							minDistance = distance;
					}

					contourSD[i] = minDistance.distance;
					if (windings[i] > 0 && minDistance.distance >= 0 && math::abs(minDistance.distance) < math::abs(posDist))
						posDist = minDistance.distance;
					if (windings[i] < 0 && minDistance.distance <= 0 && math::abs(minDistance.distance) < math::abs(negDist))
						negDist = minDistance.distance;
				}

				double sd = SignedDistance::Infinite.distance;
				if (posDist >= 0 && math::abs(posDist) <= math::abs(negDist))
				{
					sd = posDist;
					winding = 1;
					for (int i = 0; i < contourCount; ++i)
						if (windings[i] > 0 && contourSD[i] > sd && math::abs(contourSD[i]) < math::abs(negDist))
							sd = contourSD[i];
				}
				else if (negDist <= 0 && math::abs(negDist) <= math::abs(posDist))
				{
					sd = negDist;
					winding = -1;
					for (int i = 0; i < contourCount; ++i)
						if (windings[i] < 0 && contourSD[i] < sd && math::abs(contourSD[i]) < math::abs(posDist))
							sd = contourSD[i];
				}

				for (int i = 0; i < contourCount; ++i)
					if (windings[i] != winding && math::abs(contourSD[i]) < math::abs(sd))
						sd = contourSD[i];

				sd /= (range * 2.0);
				sd = math::clamp(sd, -0.5, 0.5);
				sd = sd + 0.5;

				output[x + outputPosition.x + (row + outputPosition.y) * outputStride] =
					(uint8_t)(sd * 255.0f);
			}
		}
	}

	void renderGlyph(
		const noz::import::ttf::TrueTypeFont::Glyph* glyph,
		std::vector<uint8_t>& output,
		int outputStride,
		const glm::ivec2& outputPosition,
		const glm::ivec2& outputSize,
		double range,
		const glm::dvec2& scale,
		const glm::dvec2& translate)
	{
		auto shape = Shape::fromGlyph(glyph, true);

		generateSDF(
			output,
			outputStride,
			outputPosition,
			outputSize,
			*shape,
			range,
			scale,
			translate
		);

		delete shape;
	}
}
