/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::msdf
{
	int sign(double n);
	int nonZeroSign(double n);
	double shoeLace(const glm::dvec2& a, const glm::dvec2& b);
	int solveQuadratic(double& x0, double& x1, double a, double b, double c);
	int solveCubicNormed(double& x0, double& x1, double& x2, double a, double b, double c);
	int solveCubic(double& x0, double& x1, double& x2, double a, double b, double c, double d);	
	glm::dvec2 orthoNormalize(const glm::dvec2& v, bool polarity = true);
	double cross(const glm::dvec2& lhs, const glm::dvec2& rhs);
}
