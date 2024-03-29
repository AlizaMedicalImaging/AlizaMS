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

#include "mdcmOrientation.h"
#include <cmath>

namespace mdcm
{

static const char * OrientationStrings[] = { "UNKNOWN", "AXIAL", "CORONAL", "SAGITTAL", "OBLIQUE", nullptr };

double Orientation::ObliquityThresholdCosineValue = 0.8;

Orientation::OrientationType
Orientation::GetType(const double dircos[6])
{
  OrientationType type = Orientation::UNKNOWN;
  if (dircos)
  {
    const char rowAxis = GetMajorAxisFromPatientRelativeDirectionCosine(dircos[0], dircos[1], dircos[2]);
    const char colAxis = GetMajorAxisFromPatientRelativeDirectionCosine(dircos[3], dircos[4], dircos[5]);
    if (rowAxis != 0 && colAxis != 0)
    {
      if ((rowAxis == 'R' || rowAxis == 'L') && (colAxis == 'A' || colAxis == 'P'))
        type = Orientation::AXIAL;
      else if ((colAxis == 'R' || colAxis == 'L') && (rowAxis == 'A' || rowAxis == 'P'))
        type = Orientation::AXIAL;
      else if ((rowAxis == 'R' || rowAxis == 'L') && (colAxis == 'H' || colAxis == 'F'))
        type = Orientation::CORONAL;
      else if ((colAxis == 'R' || colAxis == 'L') && (rowAxis == 'H' || rowAxis == 'F'))
        type = Orientation::CORONAL;
      else if ((rowAxis == 'A' || rowAxis == 'P') && (colAxis == 'H' || colAxis == 'F'))
        type = Orientation::SAGITTAL;
      else if ((colAxis == 'A' || colAxis == 'P') && (rowAxis == 'H' || rowAxis == 'F'))
        type = Orientation::SAGITTAL;
    }
    else
    {
      type = Orientation::OBLIQUE;
    }
  }
  return type;
}

void
Orientation::SetObliquityThresholdCosineValue(double val)
{
  Orientation::ObliquityThresholdCosineValue = val;
}

double
Orientation::GetObliquityThresholdCosineValue()
{
  return Orientation::ObliquityThresholdCosineValue;
}

const char *
Orientation::GetLabel(OrientationType type)
{
  return OrientationStrings[type];
}

char
Orientation::GetMajorAxisFromPatientRelativeDirectionCosine(double x, double y, double z)
{
  char         axis = 0;
  const char   orientationX = x < 0.0 ? 'R' : 'L';
  const char   orientationY = y < 0.0 ? 'A' : 'P';
  const char   orientationZ = z < 0.0 ? 'F' : 'H';
  const double absX = std::fabs(x);
  const double absY = std::fabs(y);
  const double absZ = std::fabs(z);
  // The tests here really don't need to check the other dimensions,
  // just the threshold, since the sum of the squares should be == 1.0
  // but just in case.
  if ((absX > ObliquityThresholdCosineValue) && (absX > absY) && (absX > absZ))
  {
    axis = orientationX;
  }
  else if ((absY > ObliquityThresholdCosineValue) && (absY > absX) && (absY > absZ))
  {
    axis = orientationY;
  }
  else if ((absZ > ObliquityThresholdCosineValue) && (absZ > absX) && (absZ > absY))
  {
    axis = orientationZ;
  }
  else
  {
    ;
  }
  return axis;
}

} // namespace mdcm
