#include "mmath.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdlib>

namespace
{

// Converts a float to a sign-magnitude-safe signed integer
std::int32_t FloatToBiasedInt(float f)
{
	static_assert(sizeof(float) == 4, "");
	std::uint32_t u;
	std::memcpy(&u, &f, sizeof(u));
	// If negative, invert the magnitude to make the range continuous across zero
	if (u & 0x80000000U)
	{
	    return static_cast<std::int32_t>(0x80000000U - u);
	}
	return static_cast<std::int32_t>(u);
}

// Converts a double to a sign-magnitude-safe signed integer
std::int64_t DoubleToBiasedInt(double d)
{
	static_assert(sizeof(double) == 8, "");
	std::uint64_t u;
	std::memcpy(&u, &d, sizeof(u));
	if (u & 0x8000000000000000ULL)
	{
	    return static_cast<std::int64_t>(0x8000000000000000ULL - u);
	}
	return static_cast<std::int64_t>(u);
}

}

bool MMath::AlmostEqual(float A, float B, float max_diff, std::uint32_t max_ulps_diff)
{
	if (std::isnan(A) || std::isnan(B)) return false;
	if (std::fabs(A - B) <= max_diff) return true;
	if (std::isinf(A) || std::isinf(B)) return A == B;
	std::int32_t iA = FloatToBiasedInt(A);
	std::int32_t iB = FloatToBiasedInt(B);
	// Using absolute difference of signed integers avoids unsigned underflow bugs
	std::uint32_t ulps_diff = static_cast<std::uint32_t>(std::abs(iA - iB));
	return ulps_diff <= max_ulps_diff;
}

bool MMath::AlmostEqual(double A, double B, double max_diff, std::uint64_t max_ulps_diff)
{
	if (std::isnan(A) || std::isnan(B)) return false;
	if (std::fabs(A - B) <= max_diff) return true;
	if (std::isinf(A) || std::isinf(B)) return A == B;
	std::int64_t iA = DoubleToBiasedInt(A);
	std::int64_t iB = DoubleToBiasedInt(B);
	std::uint64_t ulps_diff = static_cast<std::uint64_t>(std::llabs(iA - iB));
	return ulps_diff <= max_ulps_diff;
}

bool MMath::AlmostEqual(long double A, long double B, long double max_diff, std::uint64_t max_ulps_diff)
{
	if (std::isnan(A) || std::isnan(B)) return false;
	if (std::fabs(A - B) <= max_diff) return true;
	if (std::isinf(A) || std::isinf(B)) return A == B;
	// Fallback to relative epsilon comparison safely for long double in C++14
	// This avoids platform-dependent padding byte bugs with 80-bit/128-bit floats
	const long double diff = std::fabs(A - B);
	const long double max_val = std::max(std::fabs(A), std::fabs(B));
	return diff <= max_val * std::numeric_limits<long double>::epsilon() * static_cast<long double>(max_ulps_diff);
}

