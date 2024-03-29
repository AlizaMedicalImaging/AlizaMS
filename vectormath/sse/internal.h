/*
   Copyright (C) 2006, 2007 Sony Computer Entertainment Inc.
   All rights reserved.

   Redistribution and use in source and binary forms,
   with or without modification, are permitted provided that the
   following conditions are met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of the Sony Computer Entertainment Inc nor the names
    of its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef VECTORMATH_SSE_INTERNAL_HPP
#define VECTORMATH_SSE_INTERNAL_HPP

#include "allocator.h"

// 'std::bit_cast' is available in C++20, otherwise use own implementation
//#define VECTORMATH_SSE_USE_STD_BIT_CAST

#ifdef VECTORMATH_SSE_USE_STD_BIT_CAST
// C++20
#include <bit>
#else
#include <cstring>
#endif

namespace Vectormath
{
namespace SSE
{

// ========================================================
// Helper constants
// ========================================================

// Small epsilon value
static const float VECTORMATH_SLERP_TOL = 0.999f;

// Common constants used to evaluate sseSinf/cosf4/tanf4
static const float VECTORMATH_SINCOS_CC0 = -0.0013602249f;
static const float VECTORMATH_SINCOS_CC1 =  0.0416566950f;
static const float VECTORMATH_SINCOS_CC2 = -0.4999990225f;
static const float VECTORMATH_SINCOS_SC0 = -0.0001950727f;
static const float VECTORMATH_SINCOS_SC1 =  0.0083320758f;
static const float VECTORMATH_SINCOS_SC2 = -0.1666665247f;
static const float VECTORMATH_SINCOS_KC1 =  1.57079625129f;
static const float VECTORMATH_SINCOS_KC2 =  7.54978995489e-8f;

// ========================================================
// Internal helper types and functions
// ========================================================

// Shorthand functions to get the unit vectors as __m128
static inline __m128 sseUnitVec1000() { return _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f); }
static inline __m128 sseUnitVec0100() { return _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f); }
static inline __m128 sseUnitVec0010() { return _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f); }
static inline __m128 sseUnitVec0001() { return _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f); }

// Bit cast unsigned int into float
static inline float bit_cast_uint2float(unsigned int i)
{
#ifdef VECTORMATH_SSE_USE_STD_BIT_CAST
  return std::bit_cast<float>(i);
#else
  static_assert(sizeof(unsigned int) == 4 && sizeof(float) == 4, "");
  float f;
  memcpy(&f, &i, 4);
  return f;
#endif
}

VECTORMATH_ALIGNED_PRE union SSEFloat
{
  __m128 m128;
  float f[4];
} VECTORMATH_ALIGNED_POST;

// _MM_SHUFFLE requires compile-time constants
#define sseRor(vec, i) (((i) % 4) ? (_mm_shuffle_ps((vec), (vec), _MM_SHUFFLE((static_cast<unsigned char>((i) + 3) % 4), (static_cast<unsigned char>((i) + 2) % 4), (static_cast<unsigned char>((i) + 1) % 4), (static_cast<unsigned char>(i) % 4)))) : (vec))
#define sseSplat(x, e) _mm_shuffle_ps((x), (x), _MM_SHUFFLE((e), (e), (e), (e)))
#define sseSld(vec, vec2, x) sseRor((vec), ((x) / 4))

static inline __m128 sseUintToM128(unsigned int x)
{
  return _mm_set1_ps(bit_cast_uint2float(x));
}

static inline __m128 sseMAdd(__m128 a, __m128 b, __m128 c)
{
  return _mm_add_ps(c, _mm_mul_ps(a, b));
}

static inline __m128 sseMSub(__m128 a, __m128 b, __m128 c)
{
  return _mm_sub_ps(c, _mm_mul_ps(a, b));
}

static inline __m128 sseSelect(__m128 a, __m128 b, __m128 mask)
{
  return _mm_or_ps(_mm_and_ps(mask, b), _mm_andnot_ps(mask, a));
}

static inline __m128 sseSelect(__m128 a, __m128 b, unsigned int mask)
{
  return sseSelect(a, b, sseUintToM128(mask));
}

static inline __m128 sseCvtToSignedInts(__m128 x)
{
  __m128i result = _mm_cvtps_epi32(x);
  return _mm_castsi128_ps(result);
}

static inline __m128 sseCvtToFloats(__m128 x)
{
  return _mm_cvtepi32_ps(_mm_castps_si128(x));
}

static inline __m128 sseNegatef(__m128 x)
{
  return _mm_sub_ps(_mm_setzero_ps(), x);
}

static inline __m128 sseFabsf(__m128 x)
{
  return _mm_and_ps(x, sseUintToM128(0x7fffffffU));
}

static inline __m128 sseNewtonrapsonRSqrtf(__m128 x)
{
  const __m128 halfs  = _mm_setr_ps(0.5f, 0.5f, 0.5f, 0.5f);
  const __m128 threes = _mm_setr_ps(3.0f, 3.0f, 3.0f, 3.0f);
  const __m128 approx = _mm_rsqrt_ps(x);
  const __m128 muls   = _mm_mul_ps(_mm_mul_ps(x, approx), approx);
  return _mm_mul_ps(_mm_mul_ps(halfs, approx), _mm_sub_ps(threes, muls));
}

static inline __m128 sseACosf(__m128 x)
{
  const __m128 xabs = sseFabsf(x);
  const __m128 select = _mm_cmplt_ps(x, _mm_setzero_ps());
  const __m128 t1 = _mm_sqrt_ps(_mm_sub_ps(_mm_set1_ps(1.0f), xabs));

  /* Instruction counts can be reduced if the polynomial was
   * computed entirely from nested (dependent) fma's. However,
   * to reduce the number of pipeline stalls, the polygon is evaluated
   * in two halves (hi and lo).
   */
  const __m128 xabs2 = _mm_mul_ps(xabs, xabs);
  const __m128 xabs4 = _mm_mul_ps(xabs2, xabs2);

  const __m128 hi =
    sseMAdd(
      sseMAdd(
        sseMAdd(_mm_set1_ps(-0.0012624911f), xabs, _mm_set1_ps(0.0066700901f)),
          xabs, _mm_set1_ps(-0.0170881256f)),
            xabs, _mm_set1_ps(0.0308918810f));

  const __m128 lo =
    sseMAdd(
      sseMAdd(
        sseMAdd(_mm_set1_ps(-0.0501743046f), xabs, _mm_set1_ps(0.0889789874f)),
          xabs, _mm_set1_ps(-0.2145988016f)),
            xabs, _mm_set1_ps(1.5707963050f));

  const __m128 result = sseMAdd(hi, xabs4, lo);

  // Adjust the result if x is negative.
  return sseSelect(_mm_mul_ps(t1, result),                     // Positive
           sseMSub(t1, result, _mm_set1_ps(3.1415926535898f)), // Negative
             select);
}

static inline __m128 sseSinf(__m128 x)
{
  // Range reduction using : xl = angle * TwoOverPi;
  __m128 xl = _mm_mul_ps(x, _mm_set1_ps(0.63661977236f));

  // Find the quadrant the angle falls in
  // using:  q = (int) (ceil(abs(xl))*sign(xl))
  const __m128 q = sseCvtToSignedInts(xl);

  // Compute an offset based on the quadrant that the angle falls in
  const __m128 offset = _mm_and_ps(q, sseUintToM128(0x3U));

  // Remainder in range [-pi/4 .. pi/4]
  const __m128 qf = sseCvtToFloats(q);
  xl = sseMSub(qf, _mm_set1_ps(VECTORMATH_SINCOS_KC2), sseMSub(qf, _mm_set1_ps(VECTORMATH_SINCOS_KC1), x));

  // Compute x^2 and x^3
  const __m128 xl2 = _mm_mul_ps(xl, xl);
  const __m128 xl3 = _mm_mul_ps(xl2, xl);

  // Compute both the sin and cos of the angles
  // using a polynomial expression:
  //   cx = 1.0f + xl2 * ((C0 * xl2 + C1) * xl2 + C2), and
  //   sx = xl + xl3 * ((S0 * xl2 + S1) * xl2 + S2)
  const __m128 cx =
    sseMAdd(
      sseMAdd(
       sseMAdd(_mm_set1_ps(VECTORMATH_SINCOS_CC0), xl2, _mm_set1_ps(VECTORMATH_SINCOS_CC1)),
         xl2, _mm_set1_ps(VECTORMATH_SINCOS_CC2)),
           xl2, _mm_set1_ps(1.0f));
  const __m128 sx =
    sseMAdd(
      sseMAdd(
        sseMAdd(_mm_set1_ps(VECTORMATH_SINCOS_SC0), xl2, _mm_set1_ps(VECTORMATH_SINCOS_SC1)),
          xl2, _mm_set1_ps(VECTORMATH_SINCOS_SC2)),
            xl3, xl);

  // Use the cosine when the offset is odd and the sin
  // when the offset is even
  __m128 res = sseSelect(cx, sx, _mm_cmpeq_ps(_mm_and_ps(offset, sseUintToM128(0x1U)), _mm_setzero_ps()));

  // Flip the sign of the result when (offset mod 4) = 1 or 2
  return sseSelect(_mm_xor_ps(sseUintToM128(0x80000000U), res), // Negative
           res,                                                 // Positive
             _mm_cmpeq_ps(_mm_and_ps(offset, sseUintToM128(0x2U)), _mm_setzero_ps()));
}

static inline void sseSinfCosf(__m128 x, __m128 * s, __m128 * c)
{
  // Range reduction using : xl = angle * TwoOverPi;
  __m128 xl = _mm_mul_ps(x, _mm_set1_ps(0.63661977236f));

  // Find the quadrant the angle falls in
  // using:  q = (int) (ceil(abs(xl))*sign(xl))
  const __m128 q = sseCvtToSignedInts(xl);

  // Compute the offset based on the quadrant that the angle falls in.
  // Add 1 to the offset for the cosine.
  const __m128 offsetSin = _mm_and_ps(q, sseUintToM128(0x3U));
  __m128i temp = _mm_add_epi32(_mm_set1_epi32(1), _mm_castps_si128(offsetSin));
  const __m128 offsetCos = _mm_castsi128_ps(temp);

  // Remainder in range [-pi/4 .. pi/4]
  __m128 qf = sseCvtToFloats(q);
  xl = sseMSub(qf, _mm_set1_ps(VECTORMATH_SINCOS_KC2), sseMSub(qf, _mm_set1_ps(VECTORMATH_SINCOS_KC1), x));

  // Compute x^2 and x^3
  const __m128 xl2 = _mm_mul_ps(xl, xl);
  const __m128 xl3 = _mm_mul_ps(xl2, xl);

  // Compute both the sin and cos of the angles
  // using a polynomial expression:
  //   cx = 1.0f + xl2 * ((C0 * xl2 + C1) * xl2 + C2), and
  //   sx = xl + xl3 * ((S0 * xl2 + S1) * xl2 + S2)
  const __m128 cx =
    sseMAdd(
      sseMAdd(
        sseMAdd(_mm_set1_ps(VECTORMATH_SINCOS_CC0), xl2, _mm_set1_ps(VECTORMATH_SINCOS_CC1)),
          xl2, _mm_set1_ps(VECTORMATH_SINCOS_CC2)),
            xl2, _mm_set1_ps(1.0f));
  const __m128 sx =
    sseMAdd(
      sseMAdd(
        sseMAdd(_mm_set1_ps(VECTORMATH_SINCOS_SC0), xl2, _mm_set1_ps(VECTORMATH_SINCOS_SC1)),
          xl2, _mm_set1_ps(VECTORMATH_SINCOS_SC2)),
            xl3, xl);

  // Use the cosine when the offset is odd and the sin
  // when the offset is even
  __m128 sinMask = _mm_cmpeq_ps(_mm_and_ps(offsetSin, sseUintToM128(0x1U)), _mm_setzero_ps());
  __m128 cosMask = _mm_cmpeq_ps(_mm_and_ps(offsetCos, sseUintToM128(0x1U)), _mm_setzero_ps());
  *s = sseSelect(cx, sx, sinMask);
  *c = sseSelect(cx, sx, cosMask);

  // Flip the sign of the result when (offset mod 4) = 1 or 2
  sinMask = _mm_cmpeq_ps(_mm_and_ps(offsetSin, sseUintToM128(0x2U)), _mm_setzero_ps());
  cosMask = _mm_cmpeq_ps(_mm_and_ps(offsetCos, sseUintToM128(0x2U)), _mm_setzero_ps());

  *s = sseSelect(_mm_xor_ps(sseUintToM128(0x80000000U), *s), *s, sinMask);
  *c = sseSelect(_mm_xor_ps(sseUintToM128(0x80000000U), *c), *c, cosMask);
}

static inline __m128 sseVecDot3(__m128 vec0, __m128 vec1)
{
  const __m128 result = _mm_mul_ps(vec0, vec1);
  return _mm_add_ps(sseSplat(result, 0), _mm_add_ps(sseSplat(result, 1), sseSplat(result, 2)));
}

static inline __m128 sseVecDot4(__m128 vec0, __m128 vec1)
{
  const __m128 result = _mm_mul_ps(vec0, vec1);
  return _mm_add_ps(_mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 0, 0, 0)),
           _mm_add_ps(_mm_shuffle_ps(result, result, _MM_SHUFFLE(1, 1, 1, 1)),
             _mm_add_ps(_mm_shuffle_ps(result, result, _MM_SHUFFLE(2, 2, 2, 2)),
               _mm_shuffle_ps(result, result, _MM_SHUFFLE(3, 3, 3, 3)))));
}

static inline __m128 sseVecCross(__m128 vec0, __m128 vec1)
{
  const __m128 tmp0 = _mm_shuffle_ps(vec0, vec0, _MM_SHUFFLE(3, 0, 2, 1));
  const __m128 tmp1 = _mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(3, 1, 0, 2));
  const __m128 tmp2 = _mm_shuffle_ps(vec0, vec0, _MM_SHUFFLE(3, 1, 0, 2));
  const __m128 tmp3 = _mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 result = _mm_mul_ps(tmp0, tmp1);
  result = sseMSub(tmp2, tmp3, result);
  return result;
}

static inline __m128 sseVecInsert(__m128 dst, __m128 src, int slot)
{
  SSEFloat d;
  SSEFloat s;
  d.m128 = dst;
  s.m128 = src;
  d.f[slot] = s.f[slot];
  return d.m128;
}

static inline void sseVecSetElement(__m128 & vec, float scalar, int slot)
{
  (reinterpret_cast<float *>(&vec))[slot] = scalar;
}

} // namespace SSE
} // namespace Vectormath

#endif // VECTORMATH_SSE_INTERNAL_HPP
