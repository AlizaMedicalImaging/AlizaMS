#ifndef A_MMATH_H
#define A_MMATH_H

#include <limits>

class MMath
{
public:
	/*
	 * S.
	 * https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
	 *
	 * The solution is not ideal, works well in most cases.
	 *
	 * Default values are the same as in itk::Math::FloatAlmostEqual
	 * for compatibility.
	 */
	static bool AlmostEqual(
		float A, float B,
		float max_diff = 0.1f * std::numeric_limits<float>::epsilon(),
		int max_ulps_diff = 4);
	static bool AlmostEqual(
		double A, double B,
		double max_diff = 0.1 * std::numeric_limits<double>::epsilon(),
		long long max_ulps_diff = 4LL);
	static bool AlmostEqual(
		long double A, long double B,
		long double max_diff = 0.1L * std::numeric_limits<long double>::epsilon());
};

#endif
