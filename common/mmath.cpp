//#define A_MMATH_DEBUG

#include "mmath.h"
#include <cmath>
#include <cstring>
#ifdef A_MMATH_DEBUG
#include <iostream>
#endif

/*
 *
 * https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
 *
 * TODO: maybe std::isinf, std::isnan
 */

bool MMath::AlmostEqual(
	float A, float B,
	float max_diff,
	int max_ulps_diff)
{
	static_assert(sizeof(float) == 4 && sizeof(int) == 4, "");
	if (std::fabs(A - B) <= max_diff)
	{
#ifdef A_MMATH_DEBUG
		std::cout
			<< "(float) A=" << A << ", B=" << B
			<< ", std::fabs(A - B)=" << std::fabs(A - B)
			<< ", max_diff=" << max_diff << std::endl;
#endif
		return true;
	}
	if (std::signbit(A) != std::signbit(B))
	{
		return false;
	}
	int uA;
	int uB;
	std::memcpy(&uA, &A, 4);
	std::memcpy(&uB, &B, 4);
	const int ulps_diff = std::abs(uA - uB);
#ifdef A_MMATH_DEBUG
	std::cout
		<< "(float) A=" << A << ", B=" << B
		<< ", ulps_diff=" << ulps_diff
		<< ", max_ulps_diff=" << max_ulps_diff << std::endl;
#endif
	if (ulps_diff <= max_ulps_diff)
	{
		return true;
	}
	return false;
}

bool MMath::AlmostEqual(
	double A, double B,
	double max_diff,
	long long max_ulps_diff)
{
	static_assert(sizeof(double) == 8 && sizeof(long long) == 8, "");
	if (std::fabs(A - B) <= max_diff)
	{
#ifdef A_MMATH_DEBUG
		std::cout
			<< "(double) A=" << A << ", B=" << B
			<< ", std::fabs(A - B)=" << std::fabs(A - B)
			<< ", max_diff=" << max_diff << std::endl;
#endif
		return true;
	}
	if (std::signbit(A) != std::signbit(B))
	{
		return false;
	}
	long long uA;
	long long uB;
	std::memcpy(&uA, &A, 8);
	std::memcpy(&uB, &B, 8);
	const long long ulps_diff = std::abs(uA - uB);
#ifdef A_MMATH_DEBUG
	std::cout
		<< "(double) A=" << A << ", B=" << B
		<< ", ulps_diff=" << ulps_diff
		<< ", max_ulps_diff=" << max_ulps_diff << std::endl;
#endif
	if (ulps_diff <= max_ulps_diff)
	{
		return true;
	}
	return false;
}

bool MMath::AlmostEqual(
	long double A, long double B,
	long double max_diff)
{
	const long double diff = std::fabs(A - B);
	if (diff <= max_diff)
	{
#ifdef A_MMATH_DEBUG
		std::cout
			<< "(long double) A=" << A << ", B=" << B
			<< ", diff=" << diff << ", max_diff=" << max_diff << std::endl;
#endif
		return true;
	}
	const long double uA = std::fabs(A);
	const long double uB = std::fabs(B);
	const long double diff2 = ((uB > uA) ? uB : uA) * std::numeric_limits<long double>::epsilon();
#ifdef A_MMATH_DEBUG
	std::cout
		<< "(long double) A=" << A << ", B=" << B
		<< ", diff=" << diff << ", diff2=" << diff2 << std::endl;
#endif
	if (diff <= diff2)
	{
		return true;
	}
	return false;
}

#ifdef A_MMATH_DEBUG
#undef A_MMATH_DEBUG
#endif

/*
// test
int main()
{
	std::cout
		<< MMath::AlmostEqual(-0.0f, 0.0f) << '\n'
		<< MMath::AlmostEqual(-0.0, 0.0) << '\n'
		<< MMath::AlmostEqual(-0.0L, 0.0L) << '\n'
		<< MMath::AlmostEqual(11111.00000000007, 11111.00000000008) << '\n'
		<< MMath::AlmostEqual(11111111111111111.7f, 11111111111111111.8f) << '\n'
		<< MMath::AlmostEqual(11111111111111111.7L, 11111111111111111.8L) << '\n'
		<< MMath::AlmostEqual(
			std::numeric_limits<long double>::max(),
			std::numeric_limits<long double>::max() * 2.0L) << '\n'
		<< MMath::AlmostEqual(
			std::numeric_limits<long double>::epsilon() * 1e-7L,
			std::numeric_limits<long double>::min() * 1e-6L) << '\n'
		<< MMath::AlmostEqual(
			std::numeric_limits<long double>::epsilon(),
			std::numeric_limits<long double>::epsilon() - 1e-7) << '\n'
		;
	return 0;
}
*/


