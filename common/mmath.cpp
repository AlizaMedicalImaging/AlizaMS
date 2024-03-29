#include "mmath.h"
#include <cmath>
#include <cstring>

bool MMath::AlmostEqual(
	float A, float B,
	float max_diff,
	int max_ulps_diff)
{
	static_assert(sizeof(float) == 4 && sizeof(int) == 4, "");
	if (std::fabs(A - B) <= max_diff)
	{
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
		return true;
	}
	const long double uA = std::fabs(A);
	const long double uB = std::fabs(B);
	const long double diff2 =
		((uB > uA) ? uB : uA) * std::numeric_limits<long double>::epsilon();
	if (diff <= diff2)
	{
		return true;
	}
	return false;
}

