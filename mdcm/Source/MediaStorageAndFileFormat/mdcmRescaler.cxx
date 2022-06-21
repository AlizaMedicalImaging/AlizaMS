/*********************************************************
 *
 * MDCM
 *
 * Modifications github.com/issakomi
 *
 *********************************************************/

/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "mdcmRescaler.h"
#include "mdcmTrace.h"
#include "mdcmTypes.h"
#include <limits>
#include <cstring>
#include <cmath>

namespace mdcm
{

template <typename TOut, typename TIn>
void
RescaleFunction(TOut *       out,
                const TIn *  in,
                const double intercept,
                const double slope,
                const size_t s)
{
  if ((s % sizeof(TIn)) != 0)
  {
    mdcmAlwaysWarnMacro("RescaleFunction: s % sizeof(TIn) != 0");
  }
  const size_t size = s / sizeof(TIn);
  for (size_t i = 0; i < size; ++i)
  {
    out[i] = (TOut)(slope * in[i] + intercept);
  }
}

// MM: no such thing as partial specialization of function in c++,
// so instead use this trick
template <typename TOut, typename TIn>
struct FImpl;

template <typename TOut, typename TIn>
void
InverseRescaleFunction(TOut *       out,
                       const TIn *  in,
                       const double intercept,
                       const double slope,
                       const size_t s)
{
  FImpl<TOut, TIn>::InverseRescaleFunction(out, in, intercept, slope, s);
}

template <typename TOut, typename TIn>
struct FImpl
{
  static void
  InverseRescaleFunction(TOut *       out,
                         const TIn *  in,
                         const double intercept,
                         const double slope,
                         const size_t s)
  {
    if ((s % sizeof(TIn)) != 0)
    {
      mdcmAlwaysWarnMacro("InverseRescaleFunction: s % sizeof(TIn) != 0");
    }
    const size_t size = s / sizeof(TIn);
    for (size_t i = 0; i < size; ++i)
    {
      out[i] = (TOut)(((double)in[i] - intercept) / slope);
    }
  }
};

template <typename TOut>
struct FImpl<TOut, float>
{
  static void
  InverseRescaleFunction(TOut *        out,
                         const float * in,
                         const double  intercept,
                         const double  slope,
                         const size_t  s)
  {
    if ((s % sizeof(float)) != 0)
    {
      mdcmAlwaysWarnMacro("InverseRescaleFunction: s % sizeof(float) != 0");
    }
    const size_t size = s / sizeof(float);
    for (size_t i = 0; i < size; ++i)
    {
      out[i] = (TOut)llround(((double)in[i] - intercept) / slope);
    }
  }
};

template <typename TOut>
struct FImpl<TOut, double>
{
  static void
  InverseRescaleFunction(TOut *         out,
                         const double * in,
                         const double   intercept,
                         const double   slope,
                         const size_t   s)
  {
    if ((s % sizeof(double)) != 0)
    {
      mdcmAlwaysWarnMacro("InverseRescaleFunction: s % sizeof(double) != 0");
    }
    const size_t size = s / sizeof(double);
    for (size_t i = 0; i < size; ++i)
    {
      out[i] = (TOut)llround(((double)in[i] - intercept) / slope);
    }
  }
};

static PixelFormat::ScalarType
ComputeBestFit(const PixelFormat & pf, const double intercept, const double slope)
{
  if (!(pf.GetMin() < pf.GetMax()))
  {
    mdcmAlwaysWarnMacro("ComputeBestFit !(pf.GetMin() < pf.GetMax())")
  }
  if (pf == PixelFormat::SINGLEBIT)
  {
    return PixelFormat::SINGLEBIT;
  }
  if (pf == PixelFormat::FLOAT32 || pf == PixelFormat::FLOAT64)
  {
    return PixelFormat::FLOAT64;
  }
  if (fabs(((double)((long long)(intercept))) - intercept) > std::numeric_limits<double>::epsilon())
  {
    return PixelFormat::FLOAT64;
  }
  if (fabs(((double)((long long)(slope))) - slope) > std::numeric_limits<double>::epsilon())
  {
    return PixelFormat::FLOAT64;
  }
  PixelFormat::ScalarType st = PixelFormat::UNKNOWN;
  const double pfmin = slope >= 0.0 ? pf.GetMin() : pf.GetMax();
  const double pfmax = slope >= 0.0 ? pf.GetMax() : pf.GetMin();
  const double min_ = slope * pfmin + intercept;
  const double max_ = slope * pfmax + intercept;
  if (min_ >= 0)
  {
    if (max_ <= std::numeric_limits<uint8_t>::max())
    {
      st = PixelFormat::UINT8;
    }
    else if (max_ <= std::numeric_limits<uint16_t>::max())
    {
      st = PixelFormat::UINT16;
    }
    else if (max_ <= std::numeric_limits<uint32_t>::max())
    {
      st = PixelFormat::UINT32;
    }
    else if (max_ <= std::numeric_limits<uint64_t>::max())
    {
      st = PixelFormat::UINT64;
    }
    else
    {
      mdcmAlwaysWarnMacro("ComputeBestFit failed (1)");
      return st;
    }
  }
  else
  {
    if (max_ <= std::numeric_limits<int8_t>::max() && min_ >= std::numeric_limits<int8_t>::min())
    {
      st = PixelFormat::INT8;
    }
    else if (max_ <= std::numeric_limits<int16_t>::max() && min_ >= std::numeric_limits<int16_t>::min())
    {
      st = PixelFormat::INT16;
    }
    else if (max_ <= std::numeric_limits<int32_t>::max() && min_ >= std::numeric_limits<int32_t>::min())
    {
      st = PixelFormat::INT32;
    }
    else if (max_ <= std::numeric_limits<int64_t>::max() && min_ >= std::numeric_limits<int64_t>::min())
    {
      st = PixelFormat::INT64;
    }
    else
    {
      mdcmAlwaysWarnMacro("ComputeBestFit failed (2)");
      return st;
    }
  }
  return st;
}

PixelFormat::ScalarType
Rescaler::ComputeInterceptSlopePixelType()
{
  if (PF.GetSamplesPerPixel() != 1)
  {
    mdcmAlwaysWarnMacro("Samples per pixel != 1");
    assert(0);
    return PF;
  }
  PixelFormat::ScalarType output = ComputeBestFit(PF, Intercept, Slope);
  return output;
}

template <typename TIn>
void
Rescaler::RescaleFunctionIntoBestFit(char * out, const TIn * in, size_t n)
{
  const PixelFormat::ScalarType f =
    (UseTargetPixelType) ? TargetScalarType : ComputeInterceptSlopePixelType();
  switch (f)
  {
    case PixelFormat::UINT8:
      RescaleFunction<uint8_t, TIn>((uint8_t *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::INT8:
      RescaleFunction<int8_t, TIn>((int8_t *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::UINT16:
      RescaleFunction<uint16_t, TIn>((uint16_t *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::INT16:
      RescaleFunction<int16_t, TIn>((int16_t *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::UINT32:
      RescaleFunction<uint32_t, TIn>((uint32_t *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::INT32:
      RescaleFunction<int32_t, TIn>((int32_t *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::UINT64:
      RescaleFunction<uint64_t, TIn>((uint64_t *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::INT64:
      RescaleFunction<int64_t, TIn>((int64_t *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::FLOAT32:
      RescaleFunction<float, TIn>((float *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::FLOAT64:
      RescaleFunction<double, TIn>((double *)out, in, Intercept, Slope, n);
      break;
    default:
      mdcmAlwaysWarnMacro("RescaleFunctionIntoBestFit: unknown pixel format " << f);
      break;
  }
}

template <typename TIn>
void
Rescaler::InverseRescaleFunctionIntoBestFit(char * out, const TIn * in, size_t n)
{
  const PixelFormat f = ComputePixelTypeFromMinMax();
  switch (f)
  {
    case PixelFormat::UINT8:
      InverseRescaleFunction<uint8_t, TIn>((uint8_t *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::INT8:
      InverseRescaleFunction<int8_t, TIn>((int8_t *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::UINT16:
      InverseRescaleFunction<uint16_t, TIn>((uint16_t *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::INT16:
      InverseRescaleFunction<int16_t, TIn>((int16_t *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::UINT32:
      InverseRescaleFunction<uint32_t, TIn>((uint32_t *)out, in, Intercept, Slope, n);
      break;
    case PixelFormat::INT32:
      InverseRescaleFunction<int32_t, TIn>((int32_t *)out, in, Intercept, Slope, n);
      break;
    default:
      mdcmAlwaysWarnMacro("InverseRescaleFunctionIntoBestFit: unknown pixel format " << f);
      break;
  }
}

bool
Rescaler::InverseRescale(char * out, const char * in, size_t n)
{
  switch (PF)
  {
    case PixelFormat::UINT8:
      InverseRescaleFunctionIntoBestFit<uint8_t>(out, (const uint8_t *)in, n);
      break;
    case PixelFormat::INT8:
      InverseRescaleFunctionIntoBestFit<int8_t>(out, (const int8_t *)in, n);
      break;
    case PixelFormat::UINT16:
      InverseRescaleFunctionIntoBestFit<uint16_t>(out, (const uint16_t *)in, n);
      break;
    case PixelFormat::INT16:
      InverseRescaleFunctionIntoBestFit<int16_t>(out, (const int16_t *)in, n);
      break;
    case PixelFormat::UINT32:
      InverseRescaleFunctionIntoBestFit<uint32_t>(out, (const uint32_t *)in, n);
      break;
    case PixelFormat::INT32:
      InverseRescaleFunctionIntoBestFit<int32_t>(out, (const int32_t *)in, n);
      break;
    case PixelFormat::FLOAT32:
      InverseRescaleFunctionIntoBestFit<float>(out, (const float *)in, n);
      break;
    case PixelFormat::FLOAT64:
      InverseRescaleFunctionIntoBestFit<double>(out, (const double *)in, n);
      break;
    default:
      mdcmAlwaysWarnMacro("InverseRescale: unknown pixel format");
      return false;
  }
  return true;
}

bool
Rescaler::Rescale(char * out, const char * in, size_t n)
{
  switch (PF)
  {
    case PixelFormat::SINGLEBIT:
      mdcmAlwaysWarnMacro("Rescale: PixelFormat::SINGLEBIT");
      memcpy(out, in, n);
      break;
    case PixelFormat::UINT8:
      RescaleFunctionIntoBestFit<uint8_t>(out, (const uint8_t *)in, n);
      break;
    case PixelFormat::INT8:
      RescaleFunctionIntoBestFit<int8_t>(out, (const int8_t *)in, n);
      break;
    case PixelFormat::UINT12:
    case PixelFormat::UINT16:
      RescaleFunctionIntoBestFit<uint16_t>(out, (const uint16_t *)in, n);
      break;
    case PixelFormat::INT12:
    case PixelFormat::INT16:
      RescaleFunctionIntoBestFit<int16_t>(out, (const int16_t *)in, n);
      break;
    case PixelFormat::UINT32:
      RescaleFunctionIntoBestFit<uint32_t>(out, (const uint32_t *)in, n);
      break;
    case PixelFormat::INT32:
      RescaleFunctionIntoBestFit<int32_t>(out, (const int32_t *)in, n);
      break;
    case PixelFormat::UINT64:
      RescaleFunctionIntoBestFit<uint64_t>(out, (const uint64_t *)in, n);
      break;
    case PixelFormat::INT64:
      RescaleFunctionIntoBestFit<int64_t>(out, (const int64_t *)in, n);
      break;
    case PixelFormat::FLOAT32:
      RescaleFunctionIntoBestFit<float>(out, (const float *)in, n);
      break;
    case PixelFormat::FLOAT64:
      RescaleFunctionIntoBestFit<double>(out, (const double *)in, n);
      break;
    default:
      mdcmAlwaysWarnMacro("Unhandled: " << PF);
      return false;
  }
  return true;
}

static PixelFormat
ComputeInverseBestFitFromMinMax(const double intercept,
                                const double slope,
                                const double _min,
                                const double _max)
{
  PixelFormat st = PixelFormat::UNKNOWN;
  assert(_min <= _max);
  const int64_t min_ = (int64_t)((slope < 0.0) ? (_max - intercept) / slope : (_min - intercept) / slope);
  const int64_t max_ = (int64_t)((slope < 0.0) ? (_min - intercept) / slope : (_max - intercept) / slope);
  int           log2max = 0;
  if (min_ >= 0)
  {
    if (max_ <= std::numeric_limits<uint8_t>::max())
    {
      st = PixelFormat::UINT8;
    }
    else if (max_ <= std::numeric_limits<uint16_t>::max())
    {
      st = PixelFormat::UINT16;
    }
    else if (max_ <= std::numeric_limits<uint32_t>::max())
    {
      mdcmAlwaysWarnMacro("ComputeInverseBestFitFromMinMax: UINT32");
      st = PixelFormat::UINT32;
    }
    else
    {
      mdcmAlwaysWarnMacro("ComputeInverseBestFitFromMinMax failed (1)");
      return PixelFormat::UNKNOWN;
    }
    int64_t max2 = max_;
    while (max2 >>= 1)
      ++log2max;
    // need + 1 in case max == 4095 => 12bits stored required
    st.SetBitsStored((unsigned short)(log2max + 1));
  }
  else
  {
    if (max_ <= std::numeric_limits<int8_t>::max() && min_ >= std::numeric_limits<int8_t>::min())
    {
      st = PixelFormat::INT8;
    }
    else if (max_ <= std::numeric_limits<int16_t>::max() && min_ >= std::numeric_limits<int16_t>::min())
    {
      st = PixelFormat::INT16;
    }
    else if (max_ <= std::numeric_limits<int32_t>::max() && min_ >= std::numeric_limits<int32_t>::min())
    {
      mdcmAlwaysWarnMacro("ComputeInverseBestFitFromMinMax: INT32");
      st = PixelFormat::INT32;
    }
    else
    {
      mdcmAlwaysWarnMacro("ComputeInverseBestFitFromMinMax failed (2)");
      return PixelFormat::UNKNOWN;
    }
    int64_t max2 = max_ - min_;
    while (max2 >>= 1)
      ++log2max;
    const int64_t bs = log2max + 1;
    st.SetBitsStored((unsigned short)bs);
  }
  return st;
}

PixelFormat
Rescaler::ComputePixelTypeFromMinMax()
{
  return ComputeInverseBestFitFromMinMax(Intercept, Slope, ScalarRangeMin, ScalarRangeMax);
}

void
Rescaler::SetTargetPixelType(const PixelFormat & targetpf)
{
  TargetScalarType = targetpf.GetScalarType();
}

void
Rescaler::SetUseTargetPixelType(bool b)
{
  UseTargetPixelType = b;
}

void
Rescaler::SetMinMaxForPixelType(double _min, double _max)
{
  if (_min < _max)
  {
    ScalarRangeMin = _min;
    ScalarRangeMax = _max;
  }
  else
  {
    mdcmWarningMacro("min >= max, correcting");
    ScalarRangeMin = _max;
    ScalarRangeMax = _min;
  }
}

void
Rescaler::SetIntercept(double i)
{
  Intercept = i;
}

double
Rescaler::GetIntercept() const
{
  return Intercept;
}

void
Rescaler::SetSlope(double s)
{
  Slope = s;
}

double
Rescaler::GetSlope() const
{
  return Slope;
}

void
Rescaler::SetPixelFormat(const PixelFormat & pf)
{
  PF = pf;
}

} // end namespace mdcm
