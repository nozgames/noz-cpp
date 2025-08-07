/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::import::msdf
{
	struct SignedDistance
	{
		static SignedDistance Infinite;

		double distance;
		double dot;

		SignedDistance(double distance, double dot);

		bool operator< (const SignedDistance& rhs)
		{
			return math::abs(distance) < math::abs(rhs.distance) || (math::abs(distance) == math::abs(rhs.distance) && dot < rhs.dot);
		}

		bool operator> (const SignedDistance& rhs)
		{
			return math::abs(distance) > math::abs(rhs.distance) || (math::abs(distance) == math::abs(rhs.distance) && dot > rhs.dot);
		}

		bool operator <= (const SignedDistance& rhs)
		{
			return math::abs(distance) < math::abs(rhs.distance) || (math::abs(distance) == math::abs(rhs.distance) && dot <= rhs.dot);
		}

		bool operator >=(const SignedDistance& rhs)
		{
			return math::abs(distance) > math::abs(rhs.distance) || (math::abs(distance) == math::abs(rhs.distance) && dot >= rhs.dot);
		}
	};
}
