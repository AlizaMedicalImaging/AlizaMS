#ifndef A_MMATH_H
#define A_MMATH_H

#include <limits>

class MMath
{
public:
	static bool AlmostEqual(
		float A, float B,
		float max_diff = std::numeric_limits<float>::epsilon(),
		int max_ulps_diff = 4);
	static bool AlmostEqual(
		double A, double B,
		double max_diff = std::numeric_limits<double>::epsilon(),
		long long max_ulps_diff = 4);
	static bool AlmostEqual(
		long double A, long double B,
		long double max_diff = std::numeric_limits<long double>::epsilon());
};

#endif
