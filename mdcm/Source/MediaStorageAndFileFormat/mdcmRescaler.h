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

#include "mdcmTypes.h"
#include "mdcmPixelFormat.h"

namespace mdcm
{

/**
 * Rescale class
 * This class is meant to apply the linear transform of Stored Pixel Value to
 * Real World Value.
 * This is mostly found in CT or PET dataset, where the value are stored using
 * one type, but need to be converted to another scale using a linear
 * transform.
 * There are basically two cases:
 * In CT: the linear transform is generally integer based. E.g. the Stored
 * Pixel Type is unsigned short 12bits, but to get Hounsfield unit, one need to
 * apply the linear transform:  RWV = 1. * SV - 1024
 * So the best scalar to store the Real World Value will be 16 bits signed
 * type.
 *
 * In PET: the linear transform is generally floating point based.
 * Since the dynamic range can be quite high, the Rescale Slope / Rescale
 * Intercept can be changing throughout the Series. So it is important to read
 * all linear transform and deduce the best Pixel Type only at the end (when
 * all the images to be read have been parsed).
 *
 * Warning: internally any time a floating point value is found either in the
 * Rescale Slope or the Rescale Intercept it is assumed that the best matching
 * output pixel type is FLOAT64 (in previous implementation it was FLOAT32).
 * Because VR:DS is closer to a 64bits floating point type FLOAT64 is thus a
 * best matching pixel type for the floating point transformation.
 *
 * Example: input is FLOAT64, UINT16 as ouput:
 *
 *  Rescaler ir;
 *  ir.SetIntercept(0);
 *  ir.SetSlope(5.6789);
 *  ir.SetPixelFormat(FLOAT64);
 *  ir.SetMinMaxForPixelType(((PixelFormat)UINT16).GetMin(), ((PixelFormat)UINT16).GetMax());
 *  ir.InverseRescale(output,input,numberofbytes);
 *
 *
 */
class MDCM_EXPORT Rescaler
{
public:
  Rescaler()
    :
    Intercept(0),
    Slope(1),
    PF(PixelFormat::UNKNOWN),
    TargetScalarType(PixelFormat::UNKNOWN),
    ScalarRangeMin(0),
    ScalarRangeMax(0),
    UseTargetPixelType(false) {}
  ~Rescaler() {}
  PixelFormat::ScalarType ComputeInterceptSlopePixelType();
  bool InverseRescale(char *, const char *, size_t);
  bool Rescale(char *, const char *, size_t);
  PixelFormat ComputePixelTypeFromMinMax();
  void SetTargetPixelType(PixelFormat const &);
  void SetUseTargetPixelType(bool);
  void SetMinMaxForPixelType(double, double);
  void SetIntercept(double);
  double GetIntercept() const;
  void SetSlope(double);
  double GetSlope() const;
  void SetPixelFormat(PixelFormat const &);
protected:
  template<typename TIn> void RescaleFunctionIntoBestFit(
    char *, const TIn *, size_t);
  template<typename TIn> void InverseRescaleFunctionIntoBestFit(
    char *, const TIn *, size_t);
private:
  double Intercept;
  double Slope;
  PixelFormat PF;
  PixelFormat::ScalarType TargetScalarType;
  double ScalarRangeMin;
  double ScalarRangeMax;
  bool UseTargetPixelType;
};

} // end namespace mdcm

#endif //MDCMRESCALER_H
