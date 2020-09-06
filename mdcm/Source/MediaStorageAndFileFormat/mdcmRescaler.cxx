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
#include <limits>
#include <cstring>
#include <cmath>

namespace mdcm
{

template <typename TOut, typename TIn>
void RescaleFunction(
  TOut * out, const TIn * in, double intercept, double slope, size_t size)
{
  size /= sizeof(TIn);
  for(size_t i = 0; i != size; ++i)
  {
    out[i] = (TOut)(slope * in[i] + intercept);
  }
}

// no such thing as partial specialization of function in c++
// so instead use this trick
template<typename TOut, typename TIn>
struct FImpl;

template<typename TOut, typename TIn>
void InverseRescaleFunction(
  TOut * out, const TIn *in, double intercept, double slope, size_t size)
{
  FImpl<TOut,TIn>::InverseRescaleFunction(out,in,intercept,slope,size);
} // users, don't touch this

template<typename TOut, typename TIn>
struct FImpl
{
  static void InverseRescaleFunction(TOut * out, const TIn * in,
    double intercept, double slope, size_t size) // users, go ahead and specialize this
  {
    size /= sizeof(TIn); // 'size' is in bytes
    for(size_t i = 0; i != size; ++i)
    {
      out[i] = (TOut)(((double)in[i] - intercept) / slope);
    }
  }
};

// http://stackoverflow.com/questions/485525/round-for-float-in-c
// http://en.cppreference.com/w/c/numeric/math/round
template < typename T >
static inline T round_impl(const double d)
{
#ifdef MDCM_HAVE_LROUND
  // round() is C99, std::round() is C++11
  return (T)lround(d);
#else
  return (T)((d > 0.0) ? floor(d + 0.5) : ceil(d - 0.5));
#endif
}

template<typename TOut>
struct FImpl<TOut, float>
{
  static void InverseRescaleFunction(TOut * out, const float * in,
    double intercept, double slope, size_t size)
  {
    size /= sizeof(float);
    for(size_t i = 0; i != size; ++i)
    {
      out[i] = round_impl<TOut>(((double)in[i] - intercept) / slope);
    }
  }
};

template<typename TOut>
struct FImpl<TOut, double>
{
  static void InverseRescaleFunction(TOut * out, const double * in,
    double intercept, double slope, size_t size)
  {
    size /= sizeof(double);
    for(size_t i = 0; i != size; ++i)
    {
      out[i] = round_impl<TOut>(((double)in[i] - intercept) / slope);
    }
  }
};

static inline PixelFormat::ScalarType ComputeBestFit(
  const PixelFormat & pf, double intercept, double slope)
{
  PixelFormat::ScalarType st = PixelFormat::UNKNOWN;

  if (pf == PixelFormat::FLOAT16 ||
      pf == PixelFormat::FLOAT32 ||
      pf == PixelFormat::FLOAT64)
  {
    st = PixelFormat::FLOAT64;
    return st;
  }
  assert(pf.GetMin() <= pf.GetMax());
  const double pfmin = slope >= 0 ? pf.GetMin() : pf.GetMax();
  const double pfmax = slope >= 0 ? pf.GetMax() : pf.GetMin();
  const double min = slope * pfmin + intercept;
  const double max = slope * pfmax + intercept;
  if(min >= 0) // unsigned
  {
    if(max <= std::numeric_limits<uint8_t>::max())
    {
      st = PixelFormat::UINT8;
    }
    else if(max <= std::numeric_limits<uint16_t>::max())
    {
      st = PixelFormat::UINT16;
    }
    else if(max <= std::numeric_limits<uint32_t>::max())
    {
      st = PixelFormat::UINT32;
    }
    else
    {
      mdcmErrorMacro("Unhandled Pixel Format");
      return st;
    }
  }
  else
  {
    if(max <= std::numeric_limits<int8_t>::max()
     && min >= std::numeric_limits<int8_t>::min())
    {
      st = PixelFormat::INT8;
    }
    else if(max <= std::numeric_limits<int16_t>::max()
      && min >= std::numeric_limits<int16_t>::min())
    {
      st = PixelFormat::INT16;
    }
    else if(max <= std::numeric_limits<int32_t>::max()
      && min >= std::numeric_limits<int32_t>::min())
    {
      st = PixelFormat::INT32;
    }
    else
    {
      mdcmErrorMacro("Unhandled Pixel Format");
      return st;
    }
  }
  return st;
}

PixelFormat::ScalarType Rescaler::ComputeInterceptSlopePixelType()
{
  PixelFormat::ScalarType output = PixelFormat::UNKNOWN;
  if(PF == PixelFormat::SINGLEBIT)
  {
    return PixelFormat::SINGLEBIT;
  }
  if (PF == PixelFormat::FLOAT16 ||
      PF == PixelFormat::FLOAT32 ||
      PF == PixelFormat::FLOAT64)
  {
    return PixelFormat::FLOAT64;
  }
  if(Slope != (int)Slope || Intercept != (int)Intercept)
  {
    if (PF.GetBitsAllocated() <= 16)
      return PixelFormat::FLOAT32;
    else
      return PixelFormat::FLOAT64;
  }
  const double intercept = Intercept;
  const double slope = Slope;
  output = ComputeBestFit(PF,intercept,slope);
  return output;
}

template <typename TIn>
void Rescaler::RescaleFunctionIntoBestFit(char *out, const TIn *in, size_t n)
{
  double intercept = Intercept;
  double slope = Slope;
  PixelFormat::ScalarType output = ComputeInterceptSlopePixelType();
  if(UseTargetPixelType)
  {
    output = TargetScalarType;
  }
  switch(output)
  {
  case PixelFormat::UINT8:
    RescaleFunction<uint8_t,TIn>((uint8_t*)out,in,intercept,slope,n);
    break;
  case PixelFormat::INT8:
    RescaleFunction<int8_t,TIn>((int8_t*)out,in,intercept,slope,n);
    break;
  case PixelFormat::UINT16:
    RescaleFunction<uint16_t,TIn>((uint16_t*)out,in,intercept,slope,n);
    break;
  case PixelFormat::INT16:
    RescaleFunction<int16_t,TIn>((int16_t*)out,in,intercept,slope,n);
    break;
  case PixelFormat::UINT32:
    RescaleFunction<uint32_t,TIn>((uint32_t*)out,in,intercept,slope,n);
    break;
  case PixelFormat::INT32:
    RescaleFunction<int32_t,TIn>((int32_t*)out,in,intercept,slope,n);
    break;
  case PixelFormat::FLOAT16:
  case PixelFormat::FLOAT32:
    RescaleFunction<float,TIn>((float*)out,in,intercept,slope,n);
    break;
  case PixelFormat::FLOAT64:
    RescaleFunction<double,TIn>((double*)out,in,intercept,slope,n);
    break;
  default:
    break;
  }
}

template <typename TIn>
void Rescaler::InverseRescaleFunctionIntoBestFit(
  char * out, const TIn * in, size_t n)
{
  const double intercept = Intercept;
  const double slope = Slope;
  PixelFormat output = ComputePixelTypeFromMinMax();
  switch(output)
  {
  case PixelFormat::UINT8:
    InverseRescaleFunction<uint8_t,TIn>((uint8_t*)out,in,intercept,slope,n);
    break;
  case PixelFormat::INT8:
    InverseRescaleFunction<int8_t,TIn>((int8_t*)out,in,intercept,slope,n);
    break;
  case PixelFormat::UINT16:
    InverseRescaleFunction<uint16_t,TIn>((uint16_t*)out,in,intercept,slope,n);
    break;
  case PixelFormat::INT16:
    InverseRescaleFunction<int16_t,TIn>((int16_t*)out,in,intercept,slope,n);
    break;
  case PixelFormat::UINT32:
    InverseRescaleFunction<uint32_t,TIn>((uint32_t*)out,in,intercept,slope,n);
    break;
  case PixelFormat::INT32:
    InverseRescaleFunction<int32_t,TIn>((int32_t*)out,in,intercept,slope,n);
    break;
  default:
    break;
  }
}

bool Rescaler::InverseRescale(char * out, const char * in, size_t n)
{
  bool fastpath = true;
  switch(PF)
  {
  case PixelFormat::FLOAT32:
  case PixelFormat::FLOAT64:
    fastpath = false;
    break;
  default:
    break;
  }
  if(fastpath && Slope == 1 && Intercept == 0)
  {
    memcpy(out,in,n);
    return true;
  }
  switch(PF)
  {
  case PixelFormat::UINT8:
    InverseRescaleFunctionIntoBestFit<uint8_t>(out,(const uint8_t*)in,n);
    break;
  case PixelFormat::INT8:
    InverseRescaleFunctionIntoBestFit<int8_t>(out,(const int8_t*)in,n);
    break;
  case PixelFormat::UINT16:
    InverseRescaleFunctionIntoBestFit<uint16_t>(out,(const uint16_t*)in,n);
    break;
  case PixelFormat::INT16:
    InverseRescaleFunctionIntoBestFit<int16_t>(out,(const int16_t*)in,n);
    break;
  case PixelFormat::UINT32:
    InverseRescaleFunctionIntoBestFit<uint32_t>(out,(const uint32_t*)in,n);
    break;
  case PixelFormat::INT32:
    InverseRescaleFunctionIntoBestFit<int32_t>(out,(const int32_t*)in,n);
    break;
  case PixelFormat::FLOAT32:
    InverseRescaleFunctionIntoBestFit<float>(out,(const float*)in,n);
    break;
  case PixelFormat::FLOAT64:
    InverseRescaleFunctionIntoBestFit<double>(out,(const double*)in,n);
    break;
  default:
    return false;
  }
  return true;
}

bool Rescaler::Rescale(char * out, const char * in, size_t n)
{
  if(UseTargetPixelType == false)
  {
    // fast path
    if(Slope == 1 && Intercept == 0)
    {
      memcpy(out,in,n);
      return true;
    }
  }
  switch(PF)
  {
  case PixelFormat::SINGLEBIT:
    memcpy(out,in,n);
    break;
  case PixelFormat::UINT8:
    RescaleFunctionIntoBestFit<uint8_t>(out,(const uint8_t*)in,n);
    break;
  case PixelFormat::INT8:
    RescaleFunctionIntoBestFit<int8_t>(out,(const int8_t*)in,n);
    break;
  case PixelFormat::UINT12:
  case PixelFormat::UINT16:
    RescaleFunctionIntoBestFit<uint16_t>(out,(const uint16_t*)in,n);
    break;
  case PixelFormat::INT12:
  case PixelFormat::INT16:
    RescaleFunctionIntoBestFit<int16_t>(out,(const int16_t*)in,n);
    break;
  case PixelFormat::UINT32:
    RescaleFunctionIntoBestFit<uint32_t>(out,(const uint32_t*)in,n);
    break;
  case PixelFormat::INT32:
    RescaleFunctionIntoBestFit<int32_t>(out,(const int32_t*)in,n);
    break;
  case PixelFormat::FLOAT16:
  case PixelFormat::FLOAT32:
    RescaleFunctionIntoBestFit<float>(out,(const float*)in,n);
    break;
  case PixelFormat::FLOAT64:
    RescaleFunctionIntoBestFit<double>(out,(const double*)in,n);
    break;
  default:
    mdcmErrorMacro("Unhandled: " << PF);
    break;
  }
  return true;
}

static PixelFormat ComputeInverseBestFitFromMinMax(
  double intercept, double slope, double _min, double _max)
{
  PixelFormat st = PixelFormat::UNKNOWN;
  assert(_min <= _max);
  double  dmin = (_min - intercept) / slope;
  double  dmax = (_max - intercept) / slope;
  if(slope < 0)
  {
    dmin = (_max - intercept ) / slope;
    dmax = (_min - intercept ) / slope;
  }
  int64_t min  = (int64_t)dmin;
  int64_t max  = (int64_t)dmax;
  int log2max = 0;

  if(min >= 0) // unsigned
  {
    if(max <= std::numeric_limits<uint8_t>::max())
    {
      st = PixelFormat::UINT8;
    }
    else if(max <= std::numeric_limits<uint16_t>::max())
    {
      st = PixelFormat::UINT16;
    }
    else if(max <= std::numeric_limits<uint32_t>::max())
    {
      st = PixelFormat::UINT32;
    }
    else
    {
      return PixelFormat::UNKNOWN;
    }
    int64_t max2 = max;
    while (max2 >>= 1) ++log2max;
    // need + 1 in case max == 4095 => 12bits stored required
    st.SetBitsStored((unsigned short)(log2max + 1));
  }
  else
  {
    if(max <= std::numeric_limits<int8_t>::max()
      && min >= std::numeric_limits<int8_t>::min())
    {
      st = PixelFormat::INT8;
    }
    else if(max <= std::numeric_limits<int16_t>::max()
      && min >= std::numeric_limits<int16_t>::min())
    {
      st = PixelFormat::INT16;
    }
    else if(max <= std::numeric_limits<int32_t>::max()
      && min >= std::numeric_limits<int32_t>::min())
    {
      st = PixelFormat::INT32;
    }
    else
    {
      return PixelFormat::UNKNOWN;
    }
    int64_t max2 = max - min;
    while (max2 >>= 1) ++log2max;
    const int64_t bs = log2max + 1;
    st.SetBitsStored((unsigned short)bs);
  }
  return st;
}

PixelFormat Rescaler::ComputePixelTypeFromMinMax()
{
  const double intercept = Intercept;
  const double slope = Slope;
  const PixelFormat output =
    ComputeInverseBestFitFromMinMax(intercept,slope,ScalarRangeMin,ScalarRangeMax);
  return output;
}

void Rescaler::SetTargetPixelType(PixelFormat const & targetpf)
{
  TargetScalarType = targetpf.GetScalarType();
}

void Rescaler::SetUseTargetPixelType(bool b)
{
  UseTargetPixelType = b;
}

void Rescaler::SetMinMaxForPixelType(double min, double max)
{
  if(min < max)
  {
    ScalarRangeMin = min;
    ScalarRangeMax = max;
  }
  else
  {
    mdcmWarningMacro("Min > Max. Correcting");
    ScalarRangeMin = max;
    ScalarRangeMax = min;
  }
}

} // end namespace mdcm
