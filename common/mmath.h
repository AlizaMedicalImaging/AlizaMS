#ifndef A_MMATH_H
#define A_MMATH_H

#include <limits>
#include <cstdint>

// Utility for floating-point approximate comparisons.
//
// Semantics:
//  - NaN compares unequal to anything (returns false).
//  - +inf == +inf and -inf == -inf; +inf != -inf.
//  - Values with opposite signs can match if they fall within either the
//    absolute tolerance (max_diff) or the allowed ULPs distance across zero.
//  - The long-double overload always uses a relative-epsilon check scaled 
//    by max_ulps_diff to remain safe across platform-specific alignment padding.
// Precondition:
//    max_ulps_diff should be positive and reasonable (typically 1-8).
class MMath
{
public:
    static bool AlmostEqual(
        float A, float B,
        float max_diff = 2.0f * std::numeric_limits<float>::epsilon(),
        std::uint32_t max_ulps_diff = 4);
    static bool AlmostEqual(
        double A, double B,
        double max_diff = 2.0 * std::numeric_limits<double>::epsilon(),
        std::uint64_t max_ulps_diff = 4);
    static bool AlmostEqual(
        long double A, long double B,
        long double max_diff = 2.0L * std::numeric_limits<long double>::epsilon(),
        std::uint64_t max_ulps_diff = 4);
};

#endif // A_MMATH_H
