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
#ifndef MDCMRESCALER_H
#define MDCMRESCALER_H

#include "mdcmPixelFormat.h"

namespace mdcm
{

/**
 * Rescale class
 */
class MDCM_EXPORT Rescaler
{
public:
  Rescaler() = default;
  ~Rescaler() = default;
  PixelFormat::ScalarType
  ComputeInterceptSlopePixelType();
  bool
  InverseRescale(char *, const char *, size_t);
  bool
  Rescale(char *, const char *, size_t);
  PixelFormat
  ComputePixelTypeFromMinMax();
  // By default (UseTargetPixelType is false), a best
  // matching Target Pixel Type is computed.
  void
  SetTargetPixelType(const PixelFormat &);
  void
  SetUseTargetPixelType(bool);
  void
  SetMinMaxForPixelType(double, double);
  void
  SetIntercept(double);
  double
  GetIntercept() const;
  void
  SetSlope(double);
  double
  GetSlope() const;
  void
  SetPixelFormat(const PixelFormat &);

protected:
  template <typename TIn>
  void
  RescaleFunctionIntoBestFit(char *, const TIn *, size_t);
  template <typename TIn>
  void
  InverseRescaleFunctionIntoBestFit(char *, const TIn *, size_t);

private:
  double                  Intercept{0.0};
  double                  Slope{1.0};
  PixelFormat             PF{PixelFormat::UNKNOWN};
  PixelFormat::ScalarType TargetScalarType{PixelFormat::UNKNOWN};
  double                  ScalarRangeMin{};
  double                  ScalarRangeMax{};
  bool                    UseTargetPixelType{};
};

} // end namespace mdcm

#endif // MDCMRESCALER_H
