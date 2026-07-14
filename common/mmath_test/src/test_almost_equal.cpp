// test_almost_equal.cpp
#include "../../mmath.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>

static int tests_failed = 0;
static int tests_run = 0;

#define EXPECT(cond, msg) do \
{ \
	++tests_run; \
	if (!(cond)) \
	{ \
		++tests_failed; \
		std::cerr << "FAIL: " << msg << " (line " << __LINE__ << ")\n"; \
	} \
	else \
	{ \
		std::cout << "OK:   " << msg << "\n"; \
	} \
} while(0)

static uint32_t float_to_bits(float f)
{
	uint32_t b;
	std::memcpy(&b, &f, sizeof(b));
	return b;
}

static float bits_to_float(uint32_t b)
{
	float f;
	std::memcpy(&f, &b, sizeof(f));
	return f;
}

static uint64_t double_to_bits(double d)
{
	uint64_t b;
	std::memcpy(&b, &d, sizeof(b));
	return b;
}

static double bits_to_double(uint64_t b)
{
	double d;
	std::memcpy(&d, &b, sizeof(d));
	return d;
}

int main()
{
	// FLOAT tests
	{
		// +0 / -0 equality via abs-diff check
		float p0 = 0.0f;
		float n0 = -0.0f;
		EXPECT(MMath::AlmostEqual(p0, n0, 0.0f, (uint32_t)0), "float: +0 == -0 with zero max_diff");

		// +0 / -0 equality via abs-diff check
		float p1 = 0.0f;
		float n1 = -0.0f;
		EXPECT(MMath::AlmostEqual(p1, n1), "float: +0 == -0 with default");

		// same-sign ULP differences (positive)
		float a = 1.0f;
		uint32_t bits = float_to_bits(a);
		float a_next = bits_to_float(bits + 1);
		EXPECT(MMath::AlmostEqual(a, a_next, 0.0f, (uint32_t)1), "float: 1.0 vs next representable <=1 ULP");
		EXPECT(!MMath::AlmostEqual(a, a_next, 0.0f, (uint32_t)0), "float: 1.0 vs next not <=0 ULP");

		// larger ULP gap
		float a_far = bits_to_float(bits + 10);
		EXPECT(MMath::AlmostEqual(a, a_far, 0.0f, (uint32_t)10), "float: 1.0 vs +10 ULPs <=10 ULP");
		EXPECT(!MMath::AlmostEqual(a, a_far, 0.0f, (uint32_t)9), "float: 1.0 vs +10 ULPs not <=9 ULP");

		// subnormal adjacent
		float sub = std::numeric_limits<float>::denorm_min();
		float sub_next = std::nextafterf(sub, 1.0f);
		EXPECT(MMath::AlmostEqual(sub, sub_next, 0.0f, (uint32_t)1), "float: subnormal adjacent <=1 ULP");

		// max finite and previous representable
		float fmax = std::numeric_limits<float>::max();
		float fprev = std::nextafterf(fmax, 0.0f);
		EXPECT(MMath::AlmostEqual(fmax, fprev, 0.0f, (uint32_t)1), "float: max vs prev <=1 ULP");

		// max vs +inf should be false
		EXPECT(!MMath::AlmostEqual(fmax, std::numeric_limits<float>::infinity(), 0.0f, (uint32_t)1000), "float: max != +inf");

		// opposite signs (non-zero): should be false with zero max_diff
		EXPECT(!MMath::AlmostEqual(1.0f, -1.0f, 0.0f, (uint32_t)1000), "float: 1.0 vs -1.0 -> false (opposite signs)");

		// allow sign-crossing within max_diff
		EXPECT(MMath::AlmostEqual(1e-7f, -1e-7f, 1e-6f, (uint32_t)0), "float: small opposite signs equal when max_diff large enough");
	}

	// DOUBLE tests
	{
		// +0 / -0 equality via abs-diff check
		double p0 = 0.0;
		double n0 = -0.0;
		EXPECT(MMath::AlmostEqual(p0, n0, 0.0, (uint64_t)0), "double: +0 == -0 with zero max_diff");

		// +0 / -0 equality with default
		double p1 = 0.0;
		double n1 = -0.0;
		EXPECT(MMath::AlmostEqual(p1, n1), "double: +0 == -0 with default");

		// same-sign ULP differences (positive)
		double a = 1.0;
		uint64_t bits = double_to_bits(a);
		double a_next = bits_to_double(bits + 1);
		EXPECT(MMath::AlmostEqual(a, a_next, 0.0, (uint64_t)1), "double: 1.0 vs next <=1 ULP");
		EXPECT(!MMath::AlmostEqual(a, a_next, 0.0, (uint64_t)0), "double: 1.0 vs next not <=0 ULP");

		double a_far = bits_to_double(bits + 100);
		EXPECT(MMath::AlmostEqual(a, a_far, 0.0, (uint64_t)100), "double: 1.0 vs +100 ULPs <=100 ULP");
		EXPECT(!MMath::AlmostEqual(a, a_far, 0.0, (uint64_t)99), "double: 1.0 vs +100 ULPs not <=99 ULP");

		// subnormal adjacent
		double dsub = std::numeric_limits<double>::denorm_min();
		double dsub_next = std::nextafter(dsub, 1.0);
		EXPECT(MMath::AlmostEqual(dsub, dsub_next, 0.0, (uint64_t)1), "double: subnormal adjacent <=1 ULP");

		// max finite and prev
		double dmax = std::numeric_limits<double>::max();
		double dprev = std::nextafter(dmax, 0.0);
		EXPECT(MMath::AlmostEqual(dmax, dprev, 0.0, (uint64_t)1), "double: max vs prev <=1 ULP");

		// infinities
		double pinf = std::numeric_limits<double>::infinity();
		double ninf = -std::numeric_limits<double>::infinity();
		EXPECT(MMath::AlmostEqual(pinf, pinf, 0.0, (uint64_t)0), "double: +inf == +inf");
		EXPECT(!MMath::AlmostEqual(pinf, ninf, 0.0, (uint64_t)1000), "double: +inf != -inf");

		// NaN
		double nanv = std::numeric_limits<double>::quiet_NaN();
		EXPECT(!MMath::AlmostEqual(nanv, nanv, 0.0, (uint64_t)0), "double: NaN != NaN");
		EXPECT(!MMath::AlmostEqual(nanv, 1.0, 0.0, (uint64_t)1000), "double: NaN != number");

		// very large ULP gap (ensure it accepts with large max_ulps_diff)
		uint64_t big_gap = 1000000ull;
		double afar = bits_to_double(bits + big_gap);
		EXPECT(MMath::AlmostEqual(a, afar, 0.0, big_gap), "double: 1.0 vs +1e6 ULPs <=1e6 ULP");
	}

	// LONG DOUBLE tests
	{
		// +0 / -0 equality via abs-diff check
		long double p0 = 0.0L;
		long double n0 = -0.0L;
		EXPECT(MMath::AlmostEqual(p0, n0, 0.0L, (uint64_t)0), "long double: +0 == -0 with zero max_diff");

		// +0 / -0 equality with default
		long double p1 = 0.0L;
		long double n1 = -0.0L;
		EXPECT(MMath::AlmostEqual(p1, n1), "long double: +0 == -0 with default");

		long double A = 1.0L;
		const long double uA = std::fabs(A);
		const long double diff2 = uA * std::numeric_limits<long double>::epsilon();
		// choose B half the epsilon threshold away so epsilon-based equality should hold
		long double B = A + (diff2 * 0.5L);
		// We pass max_ulps_diff = 0 (so if ULPS path were used we'd require identical representations)
		bool result = MMath::AlmostEqual(A, B, 0.0L, (uint64_t)0);
		EXPECT(result, "long double fallback: small relative diff is accepted when max_ulps_diff=0 (epsilon fallback)");
	}

	// LONG DOUBLE: infinities and NaN (portable)
	{
		long double pinf = std::numeric_limits<long double>::infinity();
		long double ninf = -std::numeric_limits<long double>::infinity();
		EXPECT(MMath::AlmostEqual(pinf, pinf, 0.0L, (uint64_t)0), "long double: +inf == +inf");
		EXPECT(!MMath::AlmostEqual(pinf, ninf, 0.0L, (uint64_t)1000), "long double: +inf != -inf");

		long double nanv = std::numeric_limits<long double>::quiet_NaN();
		EXPECT(!MMath::AlmostEqual(nanv, nanv, 0.0L, (uint64_t)0), "long double: NaN != NaN");
	}

	// Summary
	std::cout << "\nTests run: " << tests_run << ", failed: " << tests_failed << "\n";
	return tests_failed == 0 ? 0 : 1;
}
