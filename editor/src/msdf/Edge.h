/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "SignedDistance.h"

namespace noz::msdf
{
	enum class EdgeColor
	{
		White
	};

	struct Edge
	{
		Edge(EdgeColor color);

		virtual glm::dvec2 point(double mix) const = 0;
		virtual void splitInThirds(std::vector<Edge*>& result) const = 0;
		virtual void bounds(double& l, double& b, double& r, double& t) const = 0;
		virtual SignedDistance distance(const glm::dvec2& origin, double& param) const = 0;

		static void bounds(const glm::dvec2& p, double& l, double& b, double& r, double& t);

		EdgeColor color;
	};

	struct LinearEdge : public Edge
	{
		LinearEdge(const glm::dvec2& p0, const glm::dvec2& p1);
		LinearEdge(const glm::dvec2& p0, const glm::dvec2& p1, EdgeColor color);

		glm::dvec2 point(double mix) const override;
		void splitInThirds(std::vector<Edge*>& result) const override;
		void bounds(double& l, double& b, double& r, double& t) const override;
		SignedDistance distance(const glm::dvec2& origin, double& param) const override;

		glm::dvec2 p0;
		glm::dvec2 p1;
	};

	struct QuadraticEdge : public Edge
	{
		QuadraticEdge(const glm::dvec2& p0, const glm::dvec2& p1, const glm::dvec2& p2);
		QuadraticEdge(const glm::dvec2& p0, const glm::dvec2& p1, const glm::dvec2& p2, EdgeColor color);

		glm::dvec2 point(double mix) const override;
		void splitInThirds(std::vector<Edge*>& result) const override;
		void bounds(double& l, double& b, double& r, double& t) const override;
		SignedDistance distance(const glm::dvec2& origin, double& param) const override;

		glm::dvec2 p0;
		glm::dvec2 p1;
		glm::dvec2 p2;

	private:

		double applySolution(
			double t,
			double oldSolution,
			const glm::dvec2& ab,
			const glm::dvec2& br,
			const glm::dvec2& origin,
			double& minDistance) const;
	};
}
