/*
  NoZ Game Engine

  Copyright(c) 2025 NoZ Games, LLC

*/

#include "../ttf/TrueTypeFont.h"
#include "Shape.h"

namespace noz::msdf
{
	bool Shape::validate()
	{
		for(auto& contour : contours)
		{
			if (contour->edges.size() == 0)
				continue;

			auto corner = contour->edges.back()->point(1.0);
			for(auto& edge : contour->edges)
			{
				auto compare = edge->point(0.0);
				if (compare != corner)
					return false;

				corner = edge->point(1.0);
			}
		}

		return true;
	}

	void Shape::normalize()
	{
		for(auto contour : contours)
		{
			if (contour->edges.size() != 1)
				continue;

			auto edge = contour->edges[0];
			contour->edges.clear();
			edge->splitInThirds(contour->edges);
			delete edge;
		}
	}

	void Shape::bounds(double& l, double& b, double& r, double& t)
	{
		for(auto contour : contours)
			contour->bounds(l, b, r, t);
	}

	Shape* Shape::fromGlyph(const ttf::TrueTypeFont::Glyph* glyph, bool invertYAxis)
	{
		if (nullptr == glyph)
			return nullptr;

		auto shape = new Shape{};
		shape->contours.resize(glyph->contours.size());

		for (size_t i = 0; i < glyph->contours.size(); i++)
		{
			auto glyphContour = glyph->contours[i];
			auto last = glyph->points[glyphContour.start].xy;
			auto start = last;

			std::vector<Edge*> edges;

			for (int p = 1; p < glyphContour.length;)
			{
				auto glyphPoint = glyph->points[glyphContour.start + p++];

				// Quadratic edge?
				if (glyphPoint.curve == ttf::TrueTypeFont::CurveType::Conic)
				{
					auto control = glyphPoint.xy;

					for (; p < glyphContour.length;)
					{
						glyphPoint = glyph->points[glyphContour.start + p++];

						if (glyphPoint.curve != ttf::TrueTypeFont::CurveType::Conic)
						{
							edges.push_back(new QuadraticEdge(
								glm::dvec2(last.x, last.y),
								glm::dvec2(control.x, control.y),
								glm::dvec2(glyphPoint.xy.x, glyphPoint.xy.y)
								));
							last = glyphPoint.xy;
							break;
						}

						auto middle = glm::dvec2((control.x + glyphPoint.xy.x) / 2, (control.y + glyphPoint.xy.y) / 2);

						edges.push_back(new QuadraticEdge(
							glm::dvec2(last.x, last.y),
							glm::dvec2(control.x, control.y),
							glm::dvec2(middle.x, middle.y)
							));

						last = middle;
						control = glyphPoint.xy;
					}

					if (p == glyphContour.length)
					{
						if (glyph->points[glyphContour.start + glyphContour.length - 1].curve == ttf::TrueTypeFont::CurveType::Conic)
						{
							edges.push_back(new QuadraticEdge(
								glm::dvec2(last.x, last.y),
								glm::dvec2(control.x, control.y),
								glm::dvec2(start.x, start.y)
								));
						}
						else
						{
							edges.push_back(new LinearEdge(
								glm::dvec2(last.x, last.y),
								glm::dvec2(start.x, start.y)
								));
						}
					}
				}
				else
				{
					edges.push_back(new LinearEdge(
						glm::dvec2(last.x, last.y),
						glm::dvec2(glyphPoint.xy.x, glyphPoint.xy.y)
						));


					last = glyphPoint.xy;

					// If we ended on a linear then finish on a linear
					if (p == glyphContour.length)
						edges.push_back(new LinearEdge(
							glm::dvec2(last.x, last.y),
							glm::dvec2(start.x, start.y)
						));
				}
			}

			auto contour = new Contour{};
			contour->edges = edges;
			shape->contours[i] = contour;
		}

		if (!shape->validate())
			throw std::exception("Invalid shape data in glyph");

		shape->normalize();
		shape->inverseYAxis = invertYAxis;

		return shape;
	}
}
